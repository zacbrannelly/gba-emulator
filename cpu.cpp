#include "cpu.h"

// =================================================================================================
// ARM - Branch and Exchange
// =================================================================================================

void cpu_arm_write_pc(CPU& cpu, uint32_t value) {
  // Make sure the last two bits are 0 (4-byte aligned)
  cpu.registers[PC] = value & ~0x3;
}

// Branch and Exchange instruction
void branch_and_exchange(uint8_t register_number, CPU& cpu) {
  uint32_t address = cpu.registers[register_number];

  if (address & 0x1) {
    // Set the T bit (5th bit, State Bit) in the CPSR
    cpu.cspr |= 0x20;

    // Make sure the last bit is 0 (2-byte aligned)
    cpu.registers[PC] = address & ~0x1;
  } else {
    // Reset the T bit (5th bit, State Bit) in the CPSR
    cpu.cspr &= ~0x20;

    // Make sure the last two bits are 0 (4-byte aligned)
    cpu_arm_write_pc(cpu, address);
  }
}

void prepare_branch_and_exchange(CPU& cpu) {
  // Add all permutations of the BX instruction
  // BX <Rm>
  // Where <Rm> is the register number (0-15)
  for (int i = 0; i < 16; i++) {
    cpu.arm_instructions[0x12FFF10 + i] = [i](CPU& cpu) { 
      branch_and_exchange(i, cpu); 
    };
  }
}

// =================================================================================================
// ARM - Branch and Branch with Link
// =================================================================================================

// Branch instruction
void branch(uint32_t offset, CPU& cpu) {
  // The offset is a signed 24-bit value
  // We need to shift it left by 2 bits (to multiply by 4)
  int32_t signed_offset = (offset << 2);

  // Then sign-extend it to 32-bits
  // Explanation: After shifting left by 2 bits, the sign bit is in the 26th bit, so we need to copy it to the most significant bits
  signed_offset |= ((offset & 0x800000) ? 0xFF000000 : 0);

  // Account for the PC being 8 bytes ahead
  // The value we get from the instruction assumes the PC is 8 bytes ahead
  uint32_t pc_with_prefetch = cpu.registers[PC] + 8;

  // Set the new PC
  cpu_arm_write_pc(cpu, pc_with_prefetch + signed_offset);
}

void branch_with_link(uint32_t offset, CPU& cpu) {
  // Set the LR to the address of the next instruction
  cpu.registers[LR] = cpu.registers[PC] + ARM_INSTRUCTION_SIZE;

  // The offset is a signed 24-bit value
  // We need to shift it left by 2 bits (to multiply by 4)
  int32_t signed_offset = (offset << 2);

  // Then sign-extend it to 32-bits
  // Explanation: After shifting left by 2 bits, the sign bit is in the 26th bit, so we need to copy it to the most significant bits
  signed_offset |= ((offset & 0x800000) ? 0xFF000000 : 0);

  // Account for the PC being 8 bytes ahead
  // The value we get from the instruction assumes the PC is 8 bytes ahead
  uint32_t pc_with_prefetch = cpu.registers[PC] + 8;

  // Set the new PC
  cpu_arm_write_pc(cpu, pc_with_prefetch + signed_offset);
}

void prepare_branch_and_link(CPU& cpu) {
  // Add all permutations of the B instruction
  // B <label>
  // Where <label> is the address to branch to
  for (int i = 0; i < 0x10000; i++) {
    cpu.arm_instructions[0xA000000 + i] = [i](CPU& cpu) { 
      branch(i, cpu);
    };
  }

  // Add all permutations of the BL instruction (Branch with Link)
  for (int i = 0; i < 0x10000; i++) {
    cpu.arm_instructions[0xB000000 + i] = [i](CPU& cpu) { 
      branch_with_link(i, cpu);
    };
  }
}

// =================================================================================================
// ARM - Data Processing
// =================================================================================================

template<bool SetFlags = false>
uint32_t shift(CPU& cpu, uint32_t value, uint8_t shift_amount, uint8_t shift_type) {
  // If LSL with shift amount 0, then the value is not shifted.
  if (shift_amount == 0 && shift_type == LOGICAL_LEFT) {
    return value;
  }

  // SPECIAL CASE: Since LSR with shift amount 0 is the same as LSL 0, we use this encoding to represent LSL with shift amount 32 instead.
  if (shift_amount == 0 && shift_type == LOGICAL_RIGHT) {
    shift_type = LOGICAL_LEFT;
    shift_amount = 32;
  }

  // SPECIAL CASE: Since ASR with shift amount 0 is the same as LSL 0, we use this encoding to represent ASR with shift amount 32 instead.
  if (shift_amount == 0 && shift_type == ARITHMETIC_RIGHT) {
    shift_type = ARITHMETIC_RIGHT;
    shift_amount = 32;
  }

  // SPECIAL CASE: Since ROR with shift amount 0 is the same as LSL 0, we use this encoding to represent a special RRX operation.
  bool is_rotate_right_extended = shift_type == ROTATE_RIGHT && shift_amount == 0;
  
  // Make sure to handle shift amounts greater than 32.
  bool overflow_shift = shift_amount > 32;

  // SPECIAL CASE: If shift amount is greater than 32 with a ROTATE_RIGHT operation, the op is equivalent to ROR with shift amount n - 32.
  if (overflow_shift && shift_type == ROTATE_RIGHT) {
    shift_type = ROTATE_RIGHT;
    while (shift_amount > 32) {
      shift_amount -= 32;
    }
  }

  switch (shift_type) {
    case LOGICAL_LEFT:
      if constexpr (SetFlags) {
        // Set the Carry flag to the last bit shifted out
        uint8_t last_bit = overflow_shift 
          ? 0 
          : (value >> (32 - shift_amount)) & 0x1;
        cpu.cspr = last_bit 
          ? cpu.cspr | 0x20000000 
          : cpu.cspr & ~0x20000000;
      }
      return overflow_shift ? 0 : value << shift_amount;
    case LOGICAL_RIGHT:
      if constexpr (SetFlags) {
        // Set the Carry flag to the last bit shifted out
        uint8_t last_bit = overflow_shift
          ? 0
          : (value >> (shift_amount - 1)) & 0x1;
        cpu.cspr = last_bit 
          ? cpu.cspr | 0x20000000 
          : cpu.cspr & ~0x20000000;
      }
      return overflow_shift ? 0 : value >> shift_amount;
    case ARITHMETIC_RIGHT:
      int32_t signed_value = (int32_t)value;
      if constexpr (SetFlags) {
        // Set the Carry flag to the last bit shifted out
        uint8_t last_bit = overflow_shift
          ? signed_value >> 31
          : (value >> (shift_amount - 1)) & 0x1;
        cpu.cspr = last_bit 
          ? cpu.cspr | 0x20000000 
          : cpu.cspr & ~0x20000000;
      }
      return overflow_shift 
        ? (signed_value < 0 ? -1 : 0)
        : (int32_t)value >> shift_amount;
    case ROTATE_RIGHT:
      if (is_rotate_right_extended) {
        // Fetch the previous Carry flag
        uint8_t carry = (cpu.cspr & 0x20000000) ? 1 : 0;

        if constexpr (SetFlags) {
          // Set the Carry flag to the last bit shifted out
          uint8_t last_bit = value & 0x1;
          cpu.cspr = last_bit 
            ? cpu.cspr | 0x20000000 
            : cpu.cspr & ~0x20000000;
        }
        
        // Perform the RRX operation - shift right by 1 and set the last bit to the previous Carry flag
        return (value >> 1) | (carry << 31);
      } else {
        if constexpr (SetFlags) {
          // Set the Carry flag to the last bit shifted out
          uint8_t last_bit = (value >> (shift_amount - 1)) & 0x1;
          cpu.cspr = last_bit 
            ? cpu.cspr | 0x20000000 
            : cpu.cspr & ~0x20000000;
        }
        return (value >> shift_amount) | (value << (32 - shift_amount));
      }
    default:
      throw std::runtime_error("Unknown shift type");
  }
}

bool shift_amount_is_in_register(uint16_t operand_2) {
  // Operand 2 is a 12-bit value
  // 4th bit signals if the shift amount is in a register (1) or an immediate value (0)
  return (operand_2 >> 4) & 0x1;
}

uint8_t get_shift_amount(CPU& cpu, uint16_t operand_2) {
  // Operand 2 is a 12-bit value
  // First 8 bits are the shift operation to be applied to the value of cpu.registers[register_operand_2].
  bool shift_is_register = shift_amount_is_in_register(operand_2);
  uint8_t shift_amount = 0;
  if (shift_is_register) {
    // Fetch the shift amount from the register, only the last 8 bits are used.
    uint8_t shift_register = (operand_2 >> 8) & 0xF;
    if (shift_register == PC) {
      throw std::runtime_error("PC register cannot be used as a shift register");
    }
    shift_amount = cpu.registers[shift_register] & 0xFF;
  } else {
    // Fetch the shift amount from the immediate value (last 5 bits)
    shift_amount = (operand_2 >> 7) & 0x1F;
  }

  return shift_amount;
}

template<bool SetFlags = false>
uint32_t apply_shift_operation(CPU& cpu, uint16_t operand_2) {
  // Last 4 bits are the register number for operand 2.
  uint8_t register_operand_2 = operand_2 & 0xF;
  uint8_t shift_amount = get_shift_amount(cpu, operand_2);
  uint8_t shift_type = (operand_2 >> 5) & 0b11;

  return shift<SetFlags>(cpu, cpu.registers[register_operand_2], shift_amount, shift_type);
}

template<bool SetFlags = false>
uint32_t apply_rotate_operation(CPU& cpu, uint16_t operand_2) {
  // The immediate is extracted from the last 8 bits and zero-extended to 32 bits.
  uint32_t operand_2_immediate = (uint32_t)(operand_2 & 0xFF);

  // The rotate amount is twice the value of the 4-bit immediate value.
  uint8_t rotate_amount = 2 * ((operand_2 >> 8) & 0xF);

  return shift<SetFlags>(cpu, operand_2_immediate, rotate_amount, ROTATE_RIGHT);
}

template<typename ResultType = uint32_t, typename SignedType = int32_t>
void update_negative_and_zero_cspr_flags(CPU& cpu, ResultType result) {
  // Set the Negative/Less Than flag if the result is negative
  if ((SignedType)result < 0) {
    cpu.cspr |= 0x80000000;
  } else {
    cpu.cspr &= ~0x80000000;
  }

  // Set the Zero flag if the result is zero
  if (result == 0) {
    cpu.cspr |= 0x40000000;
  } else {
    cpu.cspr &= ~0x40000000;
  }
}

template<bool SetFlags = false>
void and_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 & operand_2;

  if (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void exclusive_or_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 ^ operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void subtract_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 - operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);

    // Update the Carry flag
    // Carry flag is set if no borrow is needed
    // If operand_1 > operand_2, then then no borrow is needed.
    if (operand_1 >= operand_2) {
      cpu.cspr |= 0x20000000;
    } else {
      cpu.cspr &= ~0x20000000;
    }

    // Update the Overflow flag
    // sign(operand_1) != sign(operand_2) and sign(result) != sign(operand_1)
    if ((operand_1 ^ operand_2) & 0x80000000 && (operand_1 ^ result) & 0x80000000) {
      cpu.cspr |= 0x10000000;
    } else {
      cpu.cspr &= ~0x10000000;
    }
  }
}

template<bool SetFlags = false>
void reverse_subtract_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  subtract_op<SetFlags>(cpu, operand_2, operand_1, destination_register);
}

template<bool SetFlags = false>
void add_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 + operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);

    // Update the Carry flag
    // Carry flag is set if the result is less than either operand
    // Just checking one of the operands is enough
    if (result < operand_1) {
      cpu.cspr |= 0x20000000;
    } else {
      cpu.cspr &= ~0x20000000;
    }

    // Update the Overflow flag
    // sign(operand_1) == sign(operand_2) and sign(result) != sign(operand_1)
    if (((operand_1 ^ operand_2) & 0x80000000) == 0 && ((operand_1 ^ result) & 0x80000000)) {
      cpu.cspr |= 0x10000000;
    } else {
      cpu.cspr &= ~0x10000000;
    }
  }
}

template<bool SetFlags = false>
void add_with_carry_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  uint32_t carry = (cpu.cspr & 0x20000000) ? 1 : 0;
  cpu.registers[destination_register] = operand_1 + operand_2 + carry;

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);

    // Update the Carry flag
    // Carry flag is set if the result is less than either operand
    // Just checking one of the operands is enough
    if (result < operand_1) {
      cpu.cspr |= 0x20000000;
    } else {
      cpu.cspr &= ~0x20000000;
    }

    // Update the Overflow flag
    // sign(operand_1) == sign(operand_2) and sign(result) != sign(operand_1)
    if (((operand_1 ^ operand_2) & 0x80000000) == 0 && ((operand_1 ^ result) & 0x80000000)) {
      cpu.cspr |= 0x10000000;
    } else {
      cpu.cspr &= ~0x10000000;
    }
  }
}

template<bool SetFlags = false>
void subtract_with_carry_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  uint32_t carry = (cpu.cspr & 0x20000000) ? 1 : 0;
  cpu.registers[destination_register] = operand_1 - operand_2 + carry - 1;

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);

    // Update the Carry flag
    // Carry flag is set if no borrow is needed
    // If operand_1 > operand_2, then then no borrow is needed.
    // TODO: Figure out if this is correct.
    uint64_t wide_operand_1 = operand_1 + carry;
    if (wide_operand_1 >= operand_2) {
      cpu.cspr |= 0x20000000;
    } else {
      cpu.cspr &= ~0x20000000;
    }

    // Update the Overflow flag
    // sign(operand_1) != sign(operand_2) and sign(result) != sign(operand_1)
    if ((operand_1 ^ operand_2) & 0x80000000 && (operand_1 ^ result) & 0x80000000) {
      cpu.cspr |= 0x10000000;
    } else {
      cpu.cspr &= ~0x10000000;
    }
  }
}

template<bool SetFlags = false>
void reverse_subtract_with_carry_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  subtract_with_carry_op<SetFlags>(cpu, operand_2, operand_1, destination_register);
}

void test_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  // Perform the AND operation, but do not store the result
  uint32_t result = operand_1 & operand_2;
  update_negative_and_zero_cspr_flags(cpu, result);
}

void test_exclusive_or_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  // Perform the EOR operation, but do not store the result
  uint32_t result = operand_1 ^ operand_2;
  update_negative_and_zero_cspr_flags(cpu, result);
}

void compare_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  // Perform the SUB operation, but do not store the result
  uint32_t result = operand_1 - operand_2;
  update_negative_and_zero_cspr_flags(cpu, result);

  // TODO: Extract the following into a function and reuse it in the subtract_op function.

  // Update the Carry flag
  // Carry flag is set if no borrow is needed
  // If operand_1 > operand_2, then then no borrow is needed.
  if (operand_1 >= operand_2) {
    cpu.cspr |= 0x20000000;
  } else {
    cpu.cspr &= ~0x20000000;
  }

  // Update the Overflow flag
  // sign(operand_1) != sign(operand_2) and sign(result) != sign(operand_1)
  if ((operand_1 ^ operand_2) & 0x80000000 && (operand_1 ^ result) & 0x80000000) {
    cpu.cspr |= 0x10000000;
  } else {
    cpu.cspr &= ~0x10000000;
  }
}

void test_add_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  // Perform the ADD operation, but do not store the result
  uint32_t result = operand_1 + operand_2;
  update_negative_and_zero_cspr_flags(cpu, result);

  // TODO: Extract the following into a function and reuse it in the add_op function.

  // Update the Carry flag
  // Carry flag is set if the result is less than either operand
  // Just checking one of the operands is enough
  if (result < operand_1) {
    cpu.cspr |= 0x20000000;
  } else {
    cpu.cspr &= ~0x20000000;
  }

  // Update the Overflow flag
  // sign(operand_1) == sign(operand_2) and sign(result) != sign(operand_1)
  if (((operand_1 ^ operand_2) & 0x80000000) == 0 && ((operand_1 ^ result) & 0x80000000)) {
    cpu.cspr |= 0x10000000;
  } else {
    cpu.cspr &= ~0x10000000;
  }
}

template<bool SetFlags = false>
void or_operation(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 | operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void move_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags, but do not update the C and V flags
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void bit_clear_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = operand_1 & ~operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void move_not_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.registers[destination_register] = ~operand_2;

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.registers[destination_register];
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

void automatically_restore_cspr_if_applicable(CPU& cpu, uint8_t opcode) {
  // Ignore instructions that don't write to the destination register.
  if (opcode == TST || opcode == TEQ || opcode == CMP || opcode == CMN) {
    return;
  }

  // Put the saved CPU state for the current mode back 
  // into the CSPR automatically if the destination register is the PC.
  auto const mode = (cpu.cspr & 0x1F);
  auto const has_saved_cpu_state = mode != User && mode != System;
  if (has_saved_cpu_state) {
    cpu.cspr = cpu.mode_to_scspr[mode];
  }
}

template<void Op(CPU&, uint32_t, uint32_t, uint8_t), bool SetFlags = false>
void register_operation(CPU& cpu, uint8_t opcode, uint8_t operand_1_register, uint16_t operand_2, uint8_t destination_register) {
  bool shift_is_register = shift_amount_is_in_register(operand_2);
  if (shift_is_register) {
    // PC should be 12 bytes ahead if the shift amount is in a register
    // Why? Prefetching.
    cpu.registers[PC] += 3 * ARM_INSTRUCTION_SIZE;
  } else {
    // PC should be 8 bytes ahead if the shift amount is an immediate value
    // Why? Prefetching.
    cpu.registers[PC] += 2 * ARM_INSTRUCTION_SIZE;
  }

  uint32_t operand_1 = cpu.registers[operand_1_register];
  uint32_t shifted_operand_2 = apply_shift_operation<SetFlags>(cpu, operand_2);
  Op(cpu, operand_1, shifted_operand_2, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.registers[PC] -= shift_is_register ? 2 * ARM_INSTRUCTION_SIZE : ARM_INSTRUCTION_SIZE;
  }
}

template<void Op(CPU&, uint32_t, uint32_t, uint8_t), bool SetFlags = false>
void immediate_operation(CPU& cpu, uint8_t opcode, uint8_t operand_1_register, uint16_t operand_2, uint8_t destination_register) {
  // Account for prefetching.
  cpu.registers[PC] += 2 * ARM_INSTRUCTION_SIZE;
  
  uint32_t operand_1 = cpu.registers[operand_1_register];
  uint32_t operand_2_immediate = apply_rotate_operation<SetFlags>(cpu, operand_2);
  Op(cpu, operand_1, operand_2_immediate, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.registers[PC] -= ARM_INSTRUCTION_SIZE;
  }
}

constexpr uint32_t SET_CONDITIONS = 1 << 20;
constexpr uint32_t IMMEDIATE = 1 << 25;
constexpr uint32_t SOURCE_SCSPR = 1 << 22;

#define EXPAND_PERMUTATIONS(name) \
  register_operation<name<false>>, \
  immediate_operation<name<false>>, \
  register_operation<name<true>, true>, \
  immediate_operation<name<true>, true>
#define EXPAND_PERMUTATIONS_IGNORE_CSPR(name) \
  register_operation<name, true>, \
  immediate_operation<name, true>, \
  register_operation<name, true>, \
  immediate_operation<name, true>

void prepare_data_processing(CPU& cpu) {
  std::map<uint32_t, std::vector<std::function<void(CPU&, uint8_t, uint8_t, uint16_t, uint8_t)>>> op_instructions;
  op_instructions[AND] = { EXPAND_PERMUTATIONS(and_op) };
  op_instructions[EOR] = { EXPAND_PERMUTATIONS(exclusive_or_op) };
  op_instructions[SUB] = { EXPAND_PERMUTATIONS(subtract_op) };
  op_instructions[RSB] = { EXPAND_PERMUTATIONS(reverse_subtract_op) };
  op_instructions[ADD] = { EXPAND_PERMUTATIONS(add_op) };
  op_instructions[ADC] = { EXPAND_PERMUTATIONS(add_with_carry_op) };
  op_instructions[SBC] = { EXPAND_PERMUTATIONS(subtract_with_carry_op) };
  op_instructions[RSC] = { EXPAND_PERMUTATIONS(reverse_subtract_with_carry_op) };
  op_instructions[TST] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_op) };
  op_instructions[TEQ] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_exclusive_or_op) };
  op_instructions[CMP] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(compare_op) };
  op_instructions[CMN] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_add_op) };
  op_instructions[ORR] = { EXPAND_PERMUTATIONS(or_operation) };
  op_instructions[MOV] = { EXPAND_PERMUTATIONS(move_op) };
  op_instructions[BIC] = { EXPAND_PERMUTATIONS(bit_clear_op) };
  op_instructions[MVN] = { EXPAND_PERMUTATIONS(move_not_op) };

  for (int destination_register = 0; destination_register < 16; destination_register++) {
    for (int register_operand_1 = 0; register_operand_1 < 16; register_operand_1++) {
      for (uint16_t operand_2 = 0; operand_2 < 0x1000; operand_2++) {
        // Prepare the operands portion of the opcode
        // [31-28] - Condition Code (always 0 for lookup purposes)
        // [27-26] - Always 0 (unused)
        // [25] - Immediate Value (0 = Register, 1 = Immediate)
        // [24-21] - Opcode (AND, EOR, etc)
        // [20] - Set Conditions (0 = Do not set, 1 = Set)
        // [19-16] - Register Operand 1
        // [15-12] - Destination Register
        // [11-0] - Operand 2
        uint32_t const operands_code = ((register_operand_1 & 0xF) << 16) | ((destination_register & 0xF) << 12) | (operand_2 & 0xFFF);

        for (auto pair : op_instructions) {
          auto const data_opcode = pair.first;
          auto const register_op_no_cspr = pair.second[0];
          auto const immediate_op_no_cspr = pair.second[1];
          auto const register_op = pair.second[2];  
          auto const immediate_op = pair.second[3];
          
          // Add all permutations of the register instruction (no altering of the CSPR flags)
          uint32_t opcode = (data_opcode << 21) | operands_code;
          cpu.arm_instructions[opcode] = [opcode, register_operand_1, operand_2, destination_register, register_op_no_cspr](CPU& cpu) {
            register_op_no_cspr(cpu, opcode, register_operand_1, operand_2, destination_register);
          };

          // Add all permutations of the immediate instruction (no altering of the CSPR flags)
          opcode = (data_opcode << 21) | IMMEDIATE | operands_code;
          cpu.arm_instructions[opcode] = [opcode, register_operand_1, operand_2, destination_register, immediate_op_no_cspr](CPU& cpu) {
            immediate_op_no_cspr(cpu, opcode, register_operand_1, operand_2, destination_register);
          };

          // Add all permutations of the register instruction (altering the CSPR flags)
          opcode = (data_opcode << 21) | SET_CONDITIONS | operands_code;
          cpu.arm_instructions[opcode] = [opcode, register_operand_1, operand_2, destination_register, register_op](CPU& cpu) {
            register_op(cpu, opcode, register_operand_1, operand_2, destination_register);
          };

          // Add all permutations of the immediate instruction (altering the CSPR flags)
          opcode = (data_opcode << 21) | IMMEDIATE | SET_CONDITIONS | operands_code;
          cpu.arm_instructions[opcode] = [opcode, register_operand_1, operand_2, destination_register, immediate_op](CPU& cpu) {
            immediate_op(cpu, opcode, register_operand_1, operand_2, destination_register);
          };
        }
      }
    }
  }

  // MRS - Move CSPR/SCSPR to Register
  for (uint8_t destination_register = 0; destination_register < 16; destination_register++) {
    if (destination_register == PC) {
      continue;
    }
  
    // Move CSPR to Register
    uint32_t opcode = 0x10F0000 | (destination_register << 12);
    cpu.arm_instructions[opcode] = [destination_register](CPU& cpu) {
      cpu.registers[destination_register] = cpu.cspr;
    };

    // Move SCSPR to Register
    opcode = opcode | SOURCE_SCSPR;
    cpu.arm_instructions[opcode] = [destination_register](CPU& cpu) {
      cpu.registers[destination_register] = cpu.mode_to_scspr[cpu.cspr & 0x1F];
    };
  }

  // MSR - Move Register to CSPR/SCSPR
  for (uint8_t source_register = 0; source_register < 16; source_register++) {
    if (source_register == PC) {
      continue;
    }

    // Move Register to CSPR
    uint32_t opcode = 0x129F000 | source_register;
    cpu.arm_instructions[opcode] = [source_register](CPU& cpu) {
      uint8_t mode = cpu.cspr & 0x1F;
      if (mode == User) {
        cpu.cspr = cpu.registers[source_register] & 0xF0000000;
      } else {
        cpu.cspr = cpu.registers[source_register];
      }
    };

    // Move Register to SCSPR
    opcode = opcode | SOURCE_SCSPR;
    cpu.arm_instructions[opcode] = [source_register](CPU& cpu) {
      uint8_t mode = cpu.cspr & 0x1F;
      if (mode == User) {
        throw std::runtime_error("Cannot move to SCSPR in User mode");
      }
      cpu.mode_to_scspr[mode] = cpu.registers[source_register];
    };

    // Move Register to flags portion of CSPR (top 4 bits of value is moved to the top 4 bits of CSPR)
    // NOTE: CSPR is 12 bits, the middle 20 bits are reserved and should be preserved.
    opcode = 0x128F000 | source_register;
    cpu.arm_instructions[opcode] = [source_register](CPU& cpu) {
      cpu.cspr = (cpu.cspr & 0x0FFFFFFF) | (cpu.registers[source_register] & 0xF0000000);
    };
  }

  for (uint16_t operand = 0; operand < 0x1000; operand++) {
    // Move Immediate to flags portion of CSPR (top 4 bits of value is moved to the top 4 bits of CSPR)
    // NOTE: CSPR is 12 bits, the middle 20 bits are reserved and should be preserved.
    uint32_t opcode = 0x328F000 | operand;
    cpu.arm_instructions[opcode] = [operand](CPU& cpu) {
      uint32_t operand_immediate = apply_rotate_operation<false>(cpu, operand);
      cpu.cspr = (cpu.cspr & 0x0FFFFFFF) | (operand_immediate & 0xF0000000);
    };

    // Move Immediate to flags portion of SCSPR (top 4 bits of value is moved to the top 4 bits of SCSPR)
    // NOTE: CSPR is 12 bits, the middle 20 bits are reserved and should be preserved.
    opcode = opcode | SOURCE_SCSPR;
    cpu.arm_instructions[opcode] = [operand](CPU& cpu) {
      uint32_t operand_immediate = apply_rotate_operation<false>(cpu, operand);
      cpu.mode_to_scspr[cpu.cspr & 0x1F] = (cpu.mode_to_scspr[cpu.cspr & 0x1F] & 0x0FFFFFFF) | (operand_immediate & 0xF0000000);
    };
  }
}

// =================================================================================================
// ARM - Multiply
// =================================================================================================

template<bool SetFlags = false, bool Accumulate = false>
void multiply_op(CPU& cpu, uint8_t destination_register, uint8_t reg_operand_1, uint8_t reg_operand_2, uint8_t accum_reg) {
  cpu.registers[destination_register] = cpu.registers[reg_operand_1] * cpu.registers[reg_operand_2];
  if constexpr (Accumulate) {
    cpu.registers[destination_register] += cpu.registers[accum_reg];
  }
  if constexpr (SetFlags) {
    update_negative_and_zero_cspr_flags(cpu, cpu.registers[destination_register]);
  }
}

template<bool SetFlags = false, bool Accumulate = false>
void multiply_long_op(CPU& cpu, uint8_t destination_register_low, uint8_t destination_register_high, uint8_t reg_operand_1, uint8_t reg_operand_2) {
  uint64_t result = (uint64_t)cpu.registers[reg_operand_1] * (uint64_t)cpu.registers[reg_operand_2]
  if constexpr (Accumulate) {
    uint64_t accumulator = (uint64_t)cpu.registers[destination_register_high] << 32 | cpu.registers[destination_register_low];
    result += accumulator;
  }

  cpu.registers[destination_register_low] = result & 0xFFFFFFFF;
  cpu.registers[destination_register_high] = result >> 32;

  if constexpr (SetFlags) {
    update_negative_and_zero_cspr_flags<uint64_t, int64_t>(cpu, result);
  }
}

template<bool SetFlags = false, bool Accumulate = false>
void multiply_long_signed_op(CPU& cpu, uint8_t destination_register_low, uint8_t destination_register_high, uint8_t reg_operand_1, uint8_t reg_operand_2) {
  int64_t result = (int64_t)(int32_t)cpu.registers[reg_operand_1] * (int64_t)(int32_t)cpu.registers[reg_operand_2];
  if constexpr (Accumulate) {
    int64_t accumulator = (int64_t)(int32_t)cpu.registers[destination_register_high] << 32 | cpu.registers[destination_register_low];
    result += accumulator;
  }

  cpu.registers[destination_register_low] = result & 0xFFFFFFFF;
  cpu.registers[destination_register_high] = result >> 32;

  if constexpr (SetFlags) {
    update_negative_and_zero_cspr_flags<int64_t, int64_t>(cpu, result);
  }
}

void prepare_multiply(CPU& cpu) {
  // MUL - Multiply / MLA - Multiply and Accumulate
  for (uint8_t destination_register = 0; destination_register < 16; destination_register++) {
    // PC must not be the destination register.
    if (destination_register == PC) {
      continue;
    }

    for (uint8_t reg_operand_1 = 0; reg_operand_1 < 16; reg_operand_1++) {
      // PC must not be an operand.
      if (reg_operand_1 == PC) {
        continue;
      }
      
      // Rm != Rd
      if (reg_operand_1 == destination_register) {
        continue;
      }

      for (uint8_t reg_operand_2 = 0; reg_operand_2 < 16; reg_operand_2++) {
        // PC must not be an operand.
        if (reg_operand_2 == PC) {
          continue;
        }

        // Multiply only, no flags are set.
        uint32_t opcode = 0x90 | (destination_register << 16) | (reg_operand_1 << 8) | reg_operand_2;
        cpu.arm_instructions[opcode] = [destination_register, reg_operand_1, reg_operand_2](CPU& cpu) {
          multiply_op(cpu, destination_register, reg_operand_1, reg_operand_2, 0);
        };

        // Multiply only, flags are set.
        opcode = 0x90 | SET_CONDITIONS | (destination_register << 16) | (reg_operand_1 << 8) | reg_operand_2;
        cpu.arm_instructions[opcode] = [destination_register, reg_operand_1, reg_operand_2](CPU& cpu) {
          multiply_op<true>(cpu, destination_register, reg_operand_1, reg_operand_2, 0);
        };

        for (uint16_t reg_operand_3 = 0; reg_operand_3 < 16; reg_operand_3++) {
          // PC must not be an operand.
          if (reg_operand_3 == PC) {
            continue;
          }

          // Multiply and accumulate, no flags are set.
          opcode = 0x90 | (1 << 21) | (destination_register << 16) | (reg_operand_1 << 8) | reg_operand_2 | (reg_operand_3 << 12);
          cpu.arm_instructions[opcode] = [destination_register, reg_operand_1, reg_operand_2, reg_operand_3](CPU& cpu) {
            multiply_op<false, true>(cpu, destination_register, reg_operand_1, reg_operand_2, reg_operand_3);
          };

          // Multiply and accumulate, flags are set.
          opcode = 0x90 | SET_CONDITIONS | (1 << 21) | (destination_register << 16) | (reg_operand_1 << 8) | reg_operand_2 | (reg_operand_3 << 12);
          cpu.arm_instructions[opcode] = [destination_register, reg_operand_1, reg_operand_2, reg_operand_3](CPU& cpu) {
            multiply_op<true, true>(cpu, destination_register, reg_operand_1, reg_operand_2, reg_operand_3);
          }; 
        }
      }
    }
  }

  // MULL - Multiply Long / MLAL - Multiply Long and Accumulate
  for (uint8_t destination_register_low = 0; destination_register_low < 16; destination_register_low++) {
    // PC must not be the destination register.
    if (destination_register_low == PC) {
      continue;
    }

    for (uint8_t destination_register_high = 0; destination_register_high < 16; destination_register_high++) {
      // PC must not be the destination register.
      if (destination_register_high == PC) {
        continue;
      }

      // RdHi != RdLo
      if (destination_register_high == destination_register_low) {
        continue;
      }

      for (uint8_t reg_operand_1 = 0; reg_operand_1 < 16; reg_operand_1++) {
        // PC must not be an operand.
        if (reg_operand_1 == PC) {
          continue;
        }

        for (uint8_t reg_operand_2 = 0; reg_operand_2 < 16; reg_operand_2++) {
          // PC must not be an operand.
          if (reg_operand_2 == PC) {
            continue;
          }

          // Rm != RdLo and Rm != RdHi
          if (reg_operand_2 == destination_register_low || reg_operand_2 == destination_register_high) {
            continue;
          }

          // Multiply long only, unsigned, no flags are set.
          uint32_t opcode = 0x90 | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_op(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long only, unsigned, flags are set.
          opcode = 0x90 | SET_CONDITIONS | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_op<true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long only, signed, no flags are set.
          opcode = 0x90 | (1 << 22) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_signed_op(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long only, signed, flags are set.
          opcode = 0x90 | SET_CONDITIONS | (1 << 22) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_signed_op<true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long and accumulate, unsigned, no flags are set.
          opcode = 0x90 | (1 << 21) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_op<false, true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long and accumulate, unsigned, flags are set.
          opcode = 0x90 | SET_CONDITIONS | (1 << 21) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_op<true, true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long and accumulate, signed, no flags are set.
          opcode = 0x90 | (1 << 21) | (1 << 22) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_signed_op<false, true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };

          // Multiply long and accumulate, signed, flags are set.
          opcode = 0x90 | SET_CONDITIONS | (1 << 21) | (1 << 22) | (1 << 23) | (destination_register_high << 16) | (destination_register_low << 12) | (reg_operand_1 << 8) | reg_operand_2;
          cpu.arm_instructions[opcode] = [destination_register_low, destination_register_high, reg_operand_1, reg_operand_2](CPU& cpu) {
            multiply_long_signed_op<true, true>(cpu, destination_register_low, destination_register_high, reg_operand_1, reg_operand_2);
          };
        }
      }
    }
  }
}

// =================================================================================================
// ARM - Load and Store (LDR, STR)
// =================================================================================================

void prepare_load_and_store(CPU& cpu) {
  uint32_t str_base_opcode = (1 << 26);
  uint32_t ldr_base_opcode = str_base_opcode | (1 << 20);
  
  for (uint16_t offset = 0; offset < 0x1000; offset++) {
    for (uint8_t base_register = 0; base_register < 16; base_register++) {
      for (uint8_t destination_register = 0; destination_register < 16; destination_register++) {

      }
    }
  }
}

void cpu_reset(CPU& cpu) {
  // TODO: Reset registers etc.
  // TODO: Set the mode back to ARM.

  // Initialize the PC to 0.
  cpu_arm_write_pc(cpu, 0);
}

void cpu_init(CPU& cpu) {
  // Branch and Exchange (BX)
  prepare_branch_and_exchange(cpu);

  // Branch and Link (B & BL)
  prepare_branch_and_link(cpu);

  // Data Processing / PSR Transfer
  prepare_data_processing(cpu);

  // Multiply
  prepare_multiply(cpu);

  // Reset the CPU
  cpu_reset(cpu);
}

void execute_thumb_instruction(CPU& cpu, uint32_t instruction) {
  
}

bool evaluate_arm_condition(CPU& cpu, uint8_t condition) {
  // TODO: Implement the condition code evaluation
  return true;
}

void execute_arm_instruction(CPU& cpu, uint32_t instruction) {
  // Decode the condition code (most significant 4 bits of the instruction)
  uint8_t condition = instruction >> 28;

  // Evaluate the condition code
  if (!evaluate_arm_condition(cpu, condition)) {
    cpu_arm_write_pc(cpu, cpu.registers[PC] + ARM_INSTRUCTION_SIZE);
    return;
  }

  // Normalize the instruction (set the condition code to 0) so we can look it up.
  uint32_t opcode = instruction & 0x0FFFFFFF;

  // Execute the instruction (if the condition code is met)
  cpu.arm_instructions[opcode](cpu);
}

uint32_t cpu_read_next_arm_instruction(CPU& cpu) {
  // Make sure the PC is 4-byte aligned
  uint32_t pc = cpu.registers[PC] & ~0x3;

  // Fetch the instruction from the memory
  uint32_t instruction = 0;
  memcpy(&instruction, &cpu.memory[pc], ARM_INSTRUCTION_SIZE);

  return instruction;
}

uint16_t cpu_read_next_thumb_instruction(CPU& cpu) {
  // Make sure the PC is 2-byte aligned
  uint32_t pc = cpu.registers[PC] & ~0x1;

  // Fetch the instruction from the memory
  uint16_t instruction = 0;
  memcpy(&instruction, &cpu.memory[pc], THUMB_INSTRUCTION_SIZE);

  return instruction;
}

void cpu_cycle(CPU& cpu) {
  bool is_thumb = (cpu.cspr & 0x20) != 0;

  // Fetch the instruction from the memory
  uint32_t instruction = is_thumb 
    ? cpu_read_next_thumb_instruction(cpu)
    : cpu_read_next_arm_instruction(cpu);

  if (is_thumb) {
    // Decode the THUMB instruction and execute it
    execute_thumb_instruction(cpu, instruction);
  } else {
    // Decode the ARM instruction and execute it
    execute_arm_instruction(cpu, instruction);
  }
}
