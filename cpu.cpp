#include "cpu.h"

// =================================================================================================
// ARM - Branch and Exchange
// =================================================================================================

void cpu_arm_write_pc(CPU& cpu, uint32_t value) {
  // Make sure the last two bits are 0 (4-byte aligned)
  cpu.set_register_value(PC, value & ~0x3);
}

// Branch and Exchange instruction
void branch_and_exchange(uint8_t register_number, CPU& cpu) {
  uint32_t address = cpu.get_register_value(register_number);

  if (address & 0x1) {
    // Set the T bit (5th bit, State Bit) in the CPSR
    cpu.cspr |= 0x20;

    // Make sure the last bit is 0 (2-byte aligned)
    cpu.set_register_value(PC, address & ~0x1);
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
  uint32_t pc_with_prefetch = cpu.get_register_value(PC) + 8;

  // Set the new PC
  cpu_arm_write_pc(cpu, pc_with_prefetch + signed_offset);
}

void branch_with_link(uint32_t offset, CPU& cpu) {
  // Set the LR to the address of the next instruction
  cpu.set_register_value(LR, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);

  // The offset is a signed 24-bit value
  // We need to shift it left by 2 bits (to multiply by 4)
  int32_t signed_offset = (offset << 2);

  // Then sign-extend it to 32-bits
  // Explanation: After shifting left by 2 bits, the sign bit is in the 26th bit, so we need to copy it to the most significant bits
  signed_offset |= ((offset & 0x800000) ? 0xFF000000 : 0);

  // Account for the PC being 8 bytes ahead
  // The value we get from the instruction assumes the PC is 8 bytes ahead
  uint32_t pc_with_prefetch = cpu.get_register_value(PC) + 8;

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
    shift_amount = cpu.get_register_value(shift_register) & 0xFF;
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

  return shift<SetFlags>(cpu, cpu.get_register_value(register_operand_2), shift_amount, shift_type);
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
  cpu.set_register_value(destination_register, operand_1 & operand_2);

  if (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void exclusive_or_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.set_register_value(destination_register, operand_1 ^ operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void subtract_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.set_register_value(destination_register, operand_1 - operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.get_register_value(destination_register);
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
  cpu.set_register_value(destination_register, operand_1 + operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.get_register(destination_register);
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
  cpu.set_register_value(destination_register, operand_1 + operand_2 + carry);

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.get_register_value(destination_register);
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
  cpu.set_register_value(destination_register, operand_1 - operand_2 + carry - 1);

  if constexpr (SetFlags) {
    // Update the CSPR flags
    uint32_t result = cpu.get_register_value(destination_register);
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
  cpu.set_register_value(destination_register, operand_1 | operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void move_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.set_register_value(destination_register, operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags, but do not update the C and V flags
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void bit_clear_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.set_register_value(destination_register, operand_1 & ~operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

template<bool SetFlags = false>
void move_not_op(CPU& cpu, uint32_t operand_1, uint32_t operand_2, uint8_t destination_register) {
  cpu.set_register_value(destination_register, ~operand_2);

  if constexpr (SetFlags) {
    // Update the CSPR flags (only N and Z, C and V are not updated)
    uint32_t result = cpu.get_register_value(destination_register);
    update_negative_and_zero_cspr_flags(cpu, result);
  }
}

void automatically_restore_cspr_if_applicable(CPU& cpu, uint8_t opcode, uint8_t destination_register) {
  // Ignore instructions that don't write to the destination register.
  if (opcode == TST || opcode == TEQ || opcode == CMP || opcode == CMN) {
    return;
  }

  if (destination_register != PC) {
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
    cpu.set_register_value(PC, cpu.get_register_value(PC) + 3 * ARM_INSTRUCTION_SIZE);
  } else {
    // PC should be 8 bytes ahead if the shift amount is an immediate value
    // Why? Prefetching.
    cpu.set_register_value(PC, cpu.get_register_value(PC) + 2 * ARM_INSTRUCTION_SIZE);
  }

  uint32_t operand_1 = cpu.get_register_value(operand_1_register);
  uint32_t shifted_operand_2 = apply_shift_operation<SetFlags>(cpu, operand_2);
  Op(cpu, operand_1, shifted_operand_2, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode, destination_register);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.set_register_value(PC, cpu.get_register_value(PC) - shift_is_register ? 2 * ARM_INSTRUCTION_SIZE : ARM_INSTRUCTION_SIZE);
  }
}

template<void Op(CPU&, uint32_t, uint32_t, uint8_t), bool SetFlags = false>
void immediate_operation(CPU& cpu, uint8_t opcode, uint8_t operand_1_register, uint16_t operand_2, uint8_t destination_register) {
  // Account for prefetching.
  cpu.set_register_value(PC, cpu.get_register_value(PC) + 2 * ARM_INSTRUCTION_SIZE);
  
  uint32_t operand_1 = cpu.get_register_value(operand_1_register);
  uint32_t operand_2_immediate = apply_rotate_operation<SetFlags>(cpu, operand_2);
  Op(cpu, operand_1, operand_2_immediate, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode, destination_register);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.set_register_value(PC, cpu.get_register_value(PC) - ARM_INSTRUCTION_SIZE);
  }
}

static constexpr uint32_t SET_CONDITIONS = 1 << 20;
static constexpr uint32_t IMMEDIATE = 1 << 25;
static constexpr uint32_t SOURCE_SCSPR = 1 << 22;

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
      cpu.set_register_value(destination_register, cpu.cspr);
    };

    // Move SCSPR to Register
    opcode = opcode | SOURCE_SCSPR;
    cpu.arm_instructions[opcode] = [destination_register](CPU& cpu) {
      cpu.set_register_value(destination_register, cpu.mode_to_scspr[cpu.cspr & 0x1F]);
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
        cpu.cspr = cpu.get_register_value(source_register) & 0xF0000000;
      } else {
        cpu.cspr = cpu.get_register_value(source_register);
      }
    };

    // Move Register to SCSPR
    opcode = opcode | SOURCE_SCSPR;
    cpu.arm_instructions[opcode] = [source_register](CPU& cpu) {
      uint8_t mode = cpu.cspr & 0x1F;
      if (mode == User) {
        throw std::runtime_error("Cannot move to SCSPR in User mode");
      }
      cpu.mode_to_scspr[mode] = cpu.get_register_value(source_register);
    };

    // Move Register to flags portion of CSPR (top 4 bits of value is moved to the top 4 bits of CSPR)
    // NOTE: CSPR is 12 bits, the middle 20 bits are reserved and should be preserved.
    opcode = 0x128F000 | source_register;
    cpu.arm_instructions[opcode] = [source_register](CPU& cpu) {
      cpu.cspr = (cpu.cspr & 0x0FFFFFFF) | (cpu.get_register_value(source_register) & 0xF0000000);
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
  cpu.set_register_value(accum_reg, cpu.get_register_value(reg_operand_1) * cpu.get_register_value(reg_operand_2));
  if constexpr (Accumulate) {
    cpu.set_register_value(
      destination_register,
      cpu.get_register_value(destination_register) + cpu.get_register_value(accum_reg)
    );
  }
  if constexpr (SetFlags) {
    update_negative_and_zero_cspr_flags(cpu, cpu.get_register_value(destination_register));
  }
}

template<bool SetFlags = false, bool Accumulate = false>
void multiply_long_op(CPU& cpu, uint8_t destination_register_low, uint8_t destination_register_high, uint8_t reg_operand_1, uint8_t reg_operand_2) {
  uint64_t result = (uint64_t)cpu.get_register_value(reg_operand_1) * (uint64_t)cpu.get_register_value(reg_operand_2)
  if constexpr (Accumulate) {
    uint64_t accumulator = (uint64_t)cpu.get_register_value(destination_register_high) << 32 | cpu.get_register_value(destination_register_low);
    result += accumulator;
  }

  cpu.set_register_value(destination_register_low, result & 0xFFFFFFFF);
  cpu.set_register_value(destination_register_high, result >> 32);

  if constexpr (SetFlags) {
    update_negative_and_zero_cspr_flags<uint64_t, int64_t>(cpu, result);
  }
}

template<bool SetFlags = false, bool Accumulate = false>
void multiply_long_signed_op(CPU& cpu, uint8_t destination_register_low, uint8_t destination_register_high, uint8_t reg_operand_1, uint8_t reg_operand_2) {
  int64_t result = (int64_t)(int32_t)cpu.get_register_value(reg_operand_1) * (int64_t)(int32_t)cpu.get_register_value(reg_operand_2);
  if constexpr (Accumulate) {
    int64_t accumulator = (int64_t)(int32_t)cpu.get_register_value(destination_register_high) << 32 | cpu.get_register_value(destination_register_low);
    result += accumulator;
  }

  cpu.set_register_value(destination_register_low, result & 0xFFFFFFFF);
  cpu.set_register_value(destination_register_high, result >> 32);

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

// Control flags = Immediate | Pre/Post | Up/Down | Byte/Word | WriteBack (5 bits)
static constexpr uint8_t REGISTER_OFFSET = 1 << 4;
static constexpr uint8_t PRE_TRANSFER = 1 << 3;
static constexpr uint8_t UP = 1 << 2;
static constexpr uint8_t BYTE_QUANTITY = 1 << 1;
static constexpr uint8_t WRITE_BACK = 1;

void store_op(CPU& cpu, uint8_t base_register, uint8_t source_register, uint16_t offset, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  if (base_register == PC) {
    // If PC is the base register, it should be 8 bytes ahead.
    base_address += 8;
  }

  uint32_t value = cpu.get_register_value(source_register);
  if (source_register == PC) {
    // If PC is the source register, it should be 12 bytes ahead.
    value += 12;
  }

  uint32_t full_offset = 0;
  bool is_pre_transfer = control_flags & PRE_TRANSFER;

  if (control_flags & REGISTER_OFFSET) {
    full_offset = apply_shift_operation(cpu, offset);
  } else {
    full_offset = (uint32_t)offset;
  }

  if (is_pre_transfer) {
    base_address += (control_flags & UP) ? full_offset : -full_offset;
  }

  if (control_flags & BYTE_QUANTITY) {
    cpu.memory[base_address] = value & 0xFF;
  } else {
    *(uint32_t*)&cpu.memory[base_address] = value;
  }

  if (!is_pre_transfer) {
    base_address += (control_flags & UP) ? full_offset : -full_offset;
  }

  // Write back always occurs if the indexing occurs after the transfer.
  if ((control_flags & WRITE_BACK) || !is_pre_transfer) {
    cpu.set_register_value(base_register, base_address);
  }
}

void load_op(CPU& cpu, uint8_t base_register, uint8_t destination_register, uint16_t offset, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t full_offset = 0;
  bool is_pre_transfer = control_flags & PRE_TRANSFER;

  if (control_flags & REGISTER_OFFSET) {
    full_offset = apply_shift_operation(cpu, offset);
  } else {
    full_offset = (uint32_t)offset;
  }

  if (is_pre_transfer) {
    base_address += (control_flags & UP) ? full_offset : -full_offset;
  }

  // TODO: Figure out endianness
  // TODO: This needs rotation built in, unaligned addresses should be handled differently.
  // TODO: Basically you can't access memory outside a word boundary, so it rotates the word.
  // TODO: So fucking weird.
  if (control_flags & BYTE_QUANTITY) {
    cpu.set_register_value(destination_register, cpu.memory[base_address] & 0xFF);
  } else {
    uint32_t word_aligned_address = base_address & ~3;
    uint32_t word_aligned_value = *(uint32_t*)&cpu.memory[word_aligned_address];
    if (base_address != word_aligned_address) {
      uint32_t offset_from_word = base_address - word_aligned_address;
      uint32_t value = (word_aligned_value >> (offset_from_word * 8)) | (word_aligned_value << (32 - (offset_from_word * 8)));
      cpu.set_register_value(destination_register, value);
    } else {
      cpu.set_register_value(destination_register, word_aligned_value);
    }

  }

  if (!is_pre_transfer) {
    base_address += (control_flags & UP) ? full_offset : -full_offset;
  }

  // Write back always occurs if the indexing occurs after the transfer.
  if ((control_flags & WRITE_BACK) || !is_pre_transfer) {
    cpu.set_register_value(base_register, base_address);
  }
}

enum OffsetMode {
  Register,
  Immediate,
};

template<OffsetMode Mode = Register>
void load_halfword_signed_byte(CPU& cpu, uint8_t base_register, uint8_t destination_register, uint16_t offset, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t full_offset = 0;

  if (base_register == PC && control_flags & (1 << 2)) {
    throw std::runtime_error("Cannot write back to PC");
  }

  if (base_register == PC) {
    // If PC is the base register, it should be 8 bytes ahead.
    base_address += 8;
  }

  bool is_pre_transfer = control_flags & (1 << 4);
  bool is_up = control_flags & (1 << 3);

  if constexpr (Mode == Register) {
    uint8_t offset_register = offset;
    if (offset_register == PC) {
      throw std::runtime_error("Cannot use PC as offset register");
    }
    full_offset = cpu.get_register_value(offset_register);
  } else {
    full_offset = offset;
  }

  if (is_pre_transfer) {
    base_address += is_up ? full_offset : -full_offset;
  }

  uint32_t value = 0;
  
  bool is_halfword = control_flags & 1;
  bool is_signed = control_flags & 2;
  bool word_aligned = base_address & 3 == 0;
  bool halfword_aligned = base_address & 1 == 0;

  if (!word_aligned && !halfword_aligned) {
    throw std::runtime_error("Unaligned memory access :(");
  }

  if (is_halfword && !is_signed) {
    // LDRH - Load halfword
    value = *(uint16_t*)&cpu.memory[base_address];
  } else if (is_halfword && is_signed) {
    // LDRSH - Load signed halfword
    value = *(int16_t*)&cpu.memory[base_address];
  } else if (is_signed) {
    // LDRSB - Load signed byte
    value = *(int8_t*)&cpu.memory[base_address];
  } else {
    // LDRB - Load byte
    value = cpu.memory[base_address];
  }

  cpu.set_register_value(destination_register, value);

  if (!is_pre_transfer) {
    base_address += is_up ? full_offset : -full_offset;
  }

  // Write back always occurs if the indexing occurs after the transfer.
  // TODO: Confirm this in the documentation.
  bool is_write_back = control_flags & (1 << 2);
  if (is_write_back || !is_pre_transfer) {
    cpu.set_register_value(base_register, base_address);
  }
}

template<OffsetMode Mode = Register>
void store_halfword_signed_byte(CPU& cpu, uint8_t base_register, uint8_t source_register, uint16_t offset, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t full_offset = 0;

  if (base_register == PC && control_flags & (1 << 2)) {
    throw std::runtime_error("Cannot write back to PC");
  }

  if (base_register == PC) {
    // If PC is the base register, it should be 8 bytes ahead.
    base_address += 8;
  }

  bool is_pre_transfer = control_flags & (1 << 4);
  bool is_up = control_flags & (1 << 3);

  if constexpr (Mode == Register) {
    uint8_t offset_register = offset;
    full_offset = cpu.get_register_value(offset_register);
  } else {
    full_offset = offset;
  }

  if (is_pre_transfer) {
    base_address += is_up ? full_offset : -full_offset;
  }

  uint32_t value = cpu.get_register_value(source_register);

  if (source_register == PC) {
    // If PC is the source register, the value should be 12 bytes ahead.
    value += 12;
  }
  
  bool is_halfword = control_flags & 1;
  bool is_signed = control_flags & 2;

  if (is_halfword && !is_signed) {
    // STRH - Store halfword
    *(uint16_t*)&cpu.memory[base_address] = value & 0xFFFF;
  } else {
    throw std::runtime_error("Cannot store use store op on signed halfword or bytes");
  }

  if (!is_pre_transfer) {
    base_address += is_up ? full_offset : -full_offset;
  }

  // Write back always occurs if the indexing occurs after the transfer.
  // TODO: Confirm this in the documentation.
  bool is_write_back = control_flags & (1 << 2);
  if (is_write_back || !is_pre_transfer) {
    cpu.set_register_value(base_register, base_address);
  }
}

void prepare_load_and_store(CPU& cpu) {
  // Single Data Transfer (LDR, STR)
  uint32_t str_base_opcode = (1 << 26);
  uint32_t ldr_base_opcode = str_base_opcode | (1 << 20);
  
  for (uint16_t offset = 0; offset < 0x1000; offset++) {
    for (uint8_t base_register = 0; base_register < 16; base_register++) {
      for (uint8_t destination_register = 0; destination_register < 16; destination_register++) {
        for (uint8_t control_flags = 0; control_flags < 32; control_flags++) {
          uint32_t opcode = str_base_opcode | (control_flags << 21) | (base_register << 16) | (destination_register << 12) | (offset & 0xFFF);
          cpu.arm_instructions[opcode] = [base_register, destination_register, offset, control_flags](CPU& cpu) {
            store_op(cpu, base_register, destination_register, offset, control_flags);
          };

          opcode = ldr_base_opcode | (control_flags << 21) | (base_register << 16) | (destination_register << 12) | (offset & 0xFFF);
          cpu.arm_instructions[opcode] = [base_register, destination_register, offset, control_flags](CPU& cpu) {
            load_op(cpu, base_register, destination_register, offset, control_flags);
          };
        }
      }
    }
  }

  // Halfword and Signed Byte Transfer (LDRH, STRH, LDRSB, LDRSH)
  str_base_opcode = (1 << 4) | (1 << 7);
  ldr_base_opcode = str_base_opcode | (1 << 20);

  for (uint8_t base_register = 0; base_register < 16; base_register++) {
    for (uint8_t rd_register = 0; rd_register < 16; rd_register++) {
      for (uint8_t control_flags = 0; control_flags < 32; control_flags++) {
        uint32_t pu_component = (control_flags & (3 << 3)) << 23;
        uint32_t write_back_component = control_flags & (1 << 2) > 0 ? (1 << 21) : 0;
        uint32_t sh_component = (control_flags & 3) << 5;
        uint32_t control_flags_component = pu_component | write_back_component | sh_component;
        uint32_t inputs_component = (base_register << 16) | (rd_register << 12);

        for (uint8_t offset_register = 0; offset_register < 16; offset_register++) {
          // Control flags = Pre/Post | Up/Down | WriteBack | Signed | Halfword (5 bits)
          if ((control_flags & 3) == 0) {
            // Skip when Signed and Halfword are both 0 (reserved for the swp instruction)
            continue;
          }

          uint32_t opcode = str_base_opcode | control_flags_component | inputs_component | offset_register;
          if ((control_flags & 2) == 0 && (control_flags & 1)) {
            // Only allow non-signed halfword stores.
            cpu.arm_instructions[opcode] = [base_register, rd_register, offset_register, control_flags](CPU& cpu) {
              store_halfword_signed_byte(cpu, base_register, rd_register, (uint16_t)offset_register, control_flags);
            };
          }

          opcode = ldr_base_opcode | control_flags_component | inputs_component | offset_register;
          cpu.arm_instructions[opcode] = [base_register, rd_register, offset_register, control_flags](CPU& cpu) {
            load_halfword_signed_byte(cpu, base_register, rd_register, (uint16_t)offset_register, control_flags);
          };
        }

        for (uint8_t offset_immediate = 0; offset_immediate < 0x1000; offset_immediate++) {
          uint32_t offset_component = (offset_immediate & 0xF) | ((offset_immediate & 0xF0) << 4);
          uint32_t opcode = str_base_opcode | control_flags_component | inputs_component | (1 << 22) | offset_component;
          if ((control_flags & 2) == 0 && (control_flags & 1)) {
            // Only allow non-signed halfword stores.
            cpu.arm_instructions[opcode] = [base_register, rd_register, offset_immediate, control_flags](CPU& cpu) {
              store_halfword_signed_byte<Immediate>(cpu, base_register, rd_register, offset_immediate, control_flags);
            };
          }

          opcode = ldr_base_opcode | control_flags_component | inputs_component | (1 << 22) | offset_component;
          cpu.arm_instructions[opcode] = [base_register, rd_register, offset_immediate, control_flags](CPU& cpu) {
            load_halfword_signed_byte<Immediate>(cpu, base_register, rd_register, offset_immediate, control_flags);
          };
        }
      }
    }
  }
}

// =================================================================================================
// ARM - Block Data Transfer (LDM, STM)
// =================================================================================================

// Control bits - Pre/Post - Up/Down - PSR / Force User Bit - Write Back (4 bits)

void block_load(CPU& cpu, uint8_t base_register, uint16_t register_list, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t offset = (control_flags & (1 << 2)) ? 4 : -4;
  bool is_pre_transfer = control_flags & (1 << 3);
  bool write_back = control_flags & 1;
  bool load_psr = control_flags & (1 << 1);
  bool pc_in_list = register_list & (1 << 15);

  if (base_register == PC) {
    throw std::runtime_error("Cannot use PC as base register");
  }

  for (uint8_t register_idx = 0; register_idx < 16; register_idx++) {
    if (register_list & (1 << register_idx) == 0) continue;

    if (is_pre_transfer) {
      // Pre-indexing
      base_address += offset;
    }

    uint32_t value = *(uint32_t*)&cpu.memory[base_address];
    if (!pc_in_list && load_psr) {
      // Load directly into User mode registers if S bit is set and PC is not in the list.
      cpu.registers[register_idx] = value;
    } else {
      // Load to the appropriate register set.
      cpu.set_register_value(register_idx, *(uint32_t*)&cpu.memory[base_address]);
    }

    if (load_psr && register_idx == PC) {
      // Load the current modes SPSR to CSPR when S bit is set and PC is in the list.
      cpu.cspr = cpu.mode_to_scspr[cpu.cspr & 0x1F];
    }

    if (!is_pre_transfer) {
      // Post-indexing
      base_address += offset;
    }
  }

  if (write_back) {
    cpu.set_register_value(base_register, base_address);
  }
}

void block_store(CPU& cpu, uint8_t base_register, uint16_t register_list, uint8_t control_flags) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t offset = (control_flags & (1 << 2)) ? 4 : -4;
  bool is_pre_transfer = control_flags & (1 << 3);
  bool write_back = control_flags & 1;
  bool load_psr = control_flags & (1 << 1);
  bool pc_in_list = register_list & (1 << 15);

  if (base_register == PC) {
    throw std::runtime_error("Cannot use PC as base register");
  }

  for (uint8_t register_idx = 0; register_idx < 16; register_idx++) {
    if (register_list & (1 << register_idx) == 0) continue;

    if (is_pre_transfer) {
      // Pre-indexing
      base_address += offset;
    }

    if (load_psr) {
      // If S bit is set, load the User mode registers into the memory.
      *(uint32_t*)&cpu.memory[base_address] = cpu.registers[register_idx];
    } else {
      *(uint32_t*)&cpu.memory[base_address] = cpu.get_register_value(register_idx);
    }

    if (!is_pre_transfer) {
      // Post-indexing
      base_address += offset;
    }

    // Make sure write back occurs after the first register is stored.
    if (write_back) {
      cpu.set_register_value(base_register, base_address);
    }
  }
}

void prepare_block_data_transfer(CPU& cpu) {
  uint32_t stm_base_opcode = (1 << 27);
  uint32_t ldm_base_opcode = stm_base_opcode | (1 << 20);

  for (uint8_t base_register = 0; base_register < 16; base_register++) {
    for (uint16_t register_list = 0; register_list < 0x10000; register_list++) {
      for (uint8_t control_flags = 0; control_flags < 16; control_flags++) {
        uint32_t input_component = (control_flags << 21) | (base_register << 16) | register_list;
        uint32_t opcode = stm_base_opcode | input_component;
        cpu.arm_instructions[opcode] = [base_register, register_list, control_flags](CPU& cpu) {
          block_store(cpu, base_register, register_list, control_flags);
        };

        opcode = ldm_base_opcode | input_component;
        cpu.arm_instructions[opcode] = [base_register, register_list, control_flags](CPU& cpu) {
          block_load(cpu, base_register, register_list, control_flags);
        };
      }
    }
  }
}

// =================================================================================================
// ARM - Single Data Swap (SWP)
// =================================================================================================

enum SwapMode {
  SwapWord,
  SwapByte,
};

template<SwapMode Mode>
void single_data_swap(CPU& cpu, uint8_t base_register, uint8_t destination_register, uint8_t source_register) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t value = cpu.get_register_value(source_register);

  if (base_register == PC || destination_register == PC || source_register == PC) {
    throw std::runtime_error("Cannot use PC as a register in single data swap");
  }

  // Use the existing LDR and STR operations to handle the swap.
  if constexpr (Mode == SwapByte) {
    load_op(cpu, base_register, destination_register, 0, BYTE_QUANTITY);
    store_op(cpu, base_register, source_register, 0, BYTE_QUANTITY);
  } else {
    load_op(cpu, base_register, destination_register, 0, 0);
    store_op(cpu, base_register, source_register, 0, 0);
  }
}

void prepare_single_data_swap(CPU& cpu) {
  uint32_t base_word_opcode = (1 << 24) | (1 << 7) | (1 << 4);
  uint32_t base_byte_opcode = base_word_opcode | (1 << 22);

  for (uint8_t base_register = 0; base_register < 16; base_register++) {
    for (uint8_t destination_register = 0; destination_register < 16; destination_register++) {
      for (uint8_t source_register = 0; source_register < 16; source_register++) {
        uint32_t input_component = (base_register << 16) | (destination_register << 12) | source_register;
        uint32_t opcode = base_word_opcode | input_component;
        cpu.arm_instructions[opcode] = [base_register, destination_register, source_register](CPU& cpu) {
          single_data_swap<SwapWord>(cpu, base_register, destination_register, source_register);
        };

        opcode = base_byte_opcode | input_component;
        cpu.arm_instructions[opcode] = [base_register, destination_register, source_register](CPU& cpu) {
          single_data_swap<SwapByte>(cpu, base_register, destination_register, source_register);
        };
      }
    }
  }
}

// =================================================================================================
// ARM - Software Interrupt (SWI)
// =================================================================================================

void software_interrupt(CPU& cpu) {
  // Backup the current PC and CPSR
  uint32_t current_pc = cpu.get_register_value(PC);
  uint32_t current_cspr = cpu.cspr;

  // Switch to Supervisor mode
  cpu.cspr = Supervisor;

  // Save the current PC to LR, and CSPR to SPSR_svc
  cpu.set_register_value(LR, current_pc);
  cpu.mode_to_scspr[Supervisor] = current_cspr;

  // Set the PC to the SWI vector
  cpu.set_register_value(PC, 0x08);
}

// =================================================================================================
// ARM - Undefined Instruction
// =================================================================================================

void undefined_instruction(CPU& cpu) {
  throw std::runtime_error("Undefined instruction, and we have no coprocessors to handle it.");
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

  // Load and Store
  prepare_load_and_store(cpu);

  // Block Data Transfer
  prepare_block_data_transfer(cpu);

  // Single Data Swap
  prepare_single_data_swap(cpu);

  // NOTE: No need to do the coprocessor instructions as the GBA doesn't use them.
  // But we will need them to support NDS/3DS emulation in future.

  // Reset the CPU
  cpu_reset(cpu);
}

void execute_thumb_instruction(CPU& cpu, uint32_t instruction) {
  
}

bool evaluate_arm_condition(CPU& cpu, uint8_t condition) {
  switch (condition) {
    case EQ: return (cpu.cspr & CSPR_N) != 0; // Z == 1
    case NE: return (cpu.cspr & CSPR_N) == 0; // Z == 0
    case CS: return (cpu.cspr & CSPR_C) != 0; // C == 1
    case CC: return (cpu.cspr & CSPR_C) == 0; // C == 0
    case MI: return (cpu.cspr & CSPR_N) != 0; // N == 1
    case PL: return (cpu.cspr & CSPR_N) == 0; // N == 0
    case VS: return (cpu.cspr & CSPR_V) != 0; // V == 1
    case VC: return (cpu.cspr & CSPR_V) == 0; // V == 0
    case HI: return (cpu.cspr & CSPR_C) != 0 && (cpu.cspr & CSPR_Z) == 0; // C == 1 && Z == 0
    case LS: return (cpu.cspr & CSPR_C) == 0 || (cpu.cspr & CSPR_Z) != 0; // C == 0 || Z == 1
    case GE: return ((cpu.cspr & CSPR_N) > 0) == ((cpu.cspr & CSPR_V) > 0); // N == V
    case LT: return ((cpu.cspr & CSPR_N) > 0) != ((cpu.cspr & CSPR_V) > 0); // N != V
    case GT: return (cpu.cspr & CSPR_Z) == 0 && ((cpu.cspr & CSPR_N) > 0) == ((cpu.cspr & CSPR_V) > 0); // Z == 0 && N == V
    case LE: return (cpu.cspr & CSPR_Z) != 0 || ((cpu.cspr & CSPR_N) > 0) != ((cpu.cspr & CSPR_V) > 0); // Z == 1 || N != V
    case AL: return true;
    default: return false;
  }
}


void execute_arm_instruction(CPU& cpu, uint32_t instruction) {
  // Decode the condition code (most significant 4 bits of the instruction)
  uint8_t condition = instruction >> 28;

  // Evaluate the condition code
  if (!evaluate_arm_condition(cpu, condition)) {
    cpu_arm_write_pc(cpu, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
    return;
  }

  // Normalize the instruction (set the condition code to 0) so we can look it up.
  uint32_t opcode = instruction & 0x0FFFFFFF;

  // Check if the instruction is a software interrupt
  if (opcode == ARM_SOFTWARE_INTERRUPT_OPCODE) {
    software_interrupt(cpu);
    return;
  }

  if (cpu.arm_instructions.find(opcode) == cpu.arm_instructions.end()) {
    undefined_instruction(cpu);
    return;
  }

  // Execute the instruction (if the condition code is met)
  cpu.arm_instructions[opcode](cpu);
}

uint32_t cpu_read_next_arm_instruction(CPU& cpu) {
  // Make sure the PC is 4-byte aligned
  uint32_t pc = cpu.get_register_value(PC) & ~0x3;

  // Fetch the instruction from the memory
  uint32_t instruction = 0;
  memcpy(&instruction, &cpu.memory[pc], ARM_INSTRUCTION_SIZE);

  return instruction;
}

uint16_t cpu_read_next_thumb_instruction(CPU& cpu) {
  // Make sure the PC is 2-byte aligned
  uint32_t pc = cpu.get_register_value(PC) & ~0x1;

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
