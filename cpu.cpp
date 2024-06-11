#include "cpu.h"
#include <iostream>
#include <cstring>

// =================================================================================================
// Forward Declarations
// =================================================================================================
bool evaluate_arm_condition(CPU& cpu, uint8_t condition);

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

void decode_branch_and_exchange(CPU& cpu, uint32_t opcode) {
  // The register number is the last 4 bits
  uint8_t register_number = opcode & 0xF;
  branch_and_exchange(register_number, cpu);
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
  uint32_t pc_with_prefetch = cpu.get_register_value(PC) + 2 * cpu.get_instruction_size();

  // Set the new PC
  cpu_arm_write_pc(cpu, pc_with_prefetch + signed_offset);

  // TODO: Do we need to subtract the prefetch offset?
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

void decode_branch_and_link(CPU& cpu, uint32_t opcode) {
  // First 24 bits are the offset
  uint32_t offset = opcode & 0xFFFFFF;

  if (opcode & (1 << 24)) {
    // Branch with Link
    branch_with_link(offset, cpu);
  } else {
    // Branch
    branch(offset, cpu);
  }
}

// =================================================================================================
// ARM - Data Processing
// =================================================================================================

template<bool SetFlags = false, bool ImmediateInstruction = false>
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
  bool is_rotate_right_extended = shift_type == ROTATE_RIGHT && shift_amount == 0 && !ImmediateInstruction;

  // If ROR with shift amount 0, then the value is not shifted.
  // This is ignored if the special case for RRX is enabled.
  if (shift_amount == 0 && shift_type == ROTATE_RIGHT && ImmediateInstruction) {
    return value;
  }
  
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
    case LOGICAL_LEFT: {
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
    }
    case LOGICAL_RIGHT: {
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
    }
    case ARITHMETIC_RIGHT: {
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
    }
    case ROTATE_RIGHT: {
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
    }
    default: {
      throw std::runtime_error("Unknown shift type");
    }
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

template<bool SetFlags = false, bool ImmediateInstruction = false>
uint32_t apply_rotate_operation(CPU& cpu, uint16_t operand_2) {
  // The immediate is extracted from the last 8 bits and zero-extended to 32 bits.
  uint32_t operand_2_immediate = (uint32_t)(operand_2 & 0xFF);

  // The rotate amount is twice the value of the 4-bit immediate value.
  uint8_t rotate_amount = 2 * ((operand_2 >> 8) & 0xF);

  return shift<SetFlags, ImmediateInstruction>(cpu, operand_2_immediate, rotate_amount, ROTATE_RIGHT);
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
  } else {
    throw std::runtime_error("Cannot restore CSPR for User or System mode");
  }
}

template<void Op(CPU&, uint32_t, uint32_t, uint8_t), bool SetFlags = false>
void register_operation(CPU& cpu, uint8_t opcode, uint8_t operand_1_register, uint16_t operand_2, uint8_t destination_register) {
  bool shift_is_register = shift_amount_is_in_register(operand_2);
  uint8_t const instruction_size = cpu.get_instruction_size();
  if (shift_is_register) {
    // PC should be 12 bytes ahead if the shift amount is in a register
    // Why? Prefetching.
    cpu.set_register_value(PC, cpu.get_register_value(PC) + 3 * instruction_size);
  } else {
    // PC should be 8 bytes ahead if the shift amount is an immediate value
    // Why? Prefetching.
    cpu.set_register_value(PC, cpu.get_register_value(PC) + 2 * instruction_size);
  }

  uint32_t operand_1 = cpu.get_register_value(operand_1_register);
  uint32_t shifted_operand_2 = apply_shift_operation<SetFlags>(cpu, operand_2);
  Op(cpu, operand_1, shifted_operand_2, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode, destination_register);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.set_register_value(PC, cpu.get_register_value(PC) - (shift_is_register ? 2 * instruction_size : instruction_size));
  }
}

template<void Op(CPU&, uint32_t, uint32_t, uint8_t), bool SetFlags = false>
void immediate_operation(CPU& cpu, uint8_t opcode, uint8_t operand_1_register, uint16_t operand_2, uint8_t destination_register) {
  // Account for prefetching.
  uint8_t const instruction_size = cpu.get_instruction_size();
  cpu.set_register_value(PC, cpu.get_register_value(PC) + 2 * instruction_size);
  
  uint32_t operand_1 = cpu.get_register_value(operand_1_register);
  uint32_t operand_2_immediate = apply_rotate_operation<SetFlags, true /* disable special cases (e.g. RRX on ROR #0) */>(cpu, operand_2);
  Op(cpu, operand_1, operand_2_immediate, destination_register);

  if constexpr (SetFlags) {
    automatically_restore_cspr_if_applicable(cpu, opcode, destination_register);
  }

  if (destination_register != PC) {
    // Revert the prefetching offset and set the PC to the next instruction.
    cpu.set_register_value(PC, cpu.get_register_value(PC) - instruction_size);
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

void decode_move_psr_to_register(CPU& cpu, uint32_t opcode) {
  bool source_cspr = opcode & SOURCE_SCSPR;
  uint8_t destination_register = (opcode >> 12) & 0xF;
  if (source_cspr) {
    uint8_t mode = cpu.cspr & 0x1F;
    if (mode == User) {
      throw std::runtime_error("Cannot get SPSR in User mode");
    }
    cpu.set_register_value(destination_register, cpu.mode_to_scspr[mode]);
  } else {
    cpu.set_register_value(destination_register, cpu.cspr);
  }

  cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
}

void decode_move_register_to_psr(CPU& cpu, uint32_t opcode) {
  bool move_spsr = opcode & SOURCE_SCSPR;
  uint8_t source_register = opcode & 0xF;
  uint8_t mode = cpu.cspr & 0x1F;
  bool transfer_all_bits = opcode & (1 << 16);

  if (move_spsr && mode == User) {
    throw std::runtime_error("Cannot move from SPSR in User mode");
  }

  // Only the flag bits are being changed.
  if (!transfer_all_bits) {
    // NOTE: CSPR is 12 bits, the middle 20 bits are reserved and should be preserved.
    bool immediate = opcode & (1 << 29);
    if (immediate) {
      uint32_t source_operand = opcode & 0xFFF;
      uint32_t operand_immediate = apply_rotate_operation<false>(cpu, source_operand);
      if (move_spsr) {
        cpu.mode_to_scspr[mode] = (cpu.mode_to_scspr[mode] & 0x0FFFFFFF) | (operand_immediate & 0xF0000000);
      } else {
        cpu.cspr = (cpu.cspr & 0x0FFFFFFF) | (operand_immediate & 0xF0000000);
      }
    } else {
      if (move_spsr) {
        cpu.mode_to_scspr[mode] = (cpu.mode_to_scspr[mode] & 0x0FFFFFFF) | (cpu.get_register_value(source_register) & 0xF0000000);
      } else {
        cpu.cspr = (cpu.cspr & 0x0FFFFFFF) | (cpu.get_register_value(source_register) & 0xF0000000);
      }
    }
    // Increment the PC to the next instruction
    cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
    return;
  }

  if (move_spsr) {
    cpu.mode_to_scspr[mode] = cpu.get_register_value(source_register);
  } else if (mode == User) {
    cpu.cspr = cpu.get_register_value(source_register) & 0xF0000000;
  } else {
    cpu.cspr = cpu.get_register_value(source_register);
  }

  // Increment the PC to the next instruction
  cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
}

static constexpr uint32_t REGISTER_AND_NO_CSPR = 0;
static constexpr uint32_t IMMEDIATE_AND_NO_CSPR = 1;
static constexpr uint32_t REGISTER_AND_CSPR = 2;
static constexpr uint32_t IMMEDIATE_AND_CSPR = 3;

void decode_data_processing(CPU& cpu, uint32_t opcode) {
  uint8_t data_opcode = (opcode >> 21) & 0xF;
  bool set_conditions = opcode & SET_CONDITIONS;

  // Special case: MRS - Move PSR to Register (TST | CMP with SET_CONDITIONS = 0)
  if (!set_conditions && (data_opcode == TST || data_opcode == CMP)) {
    decode_move_psr_to_register(cpu, opcode);
    return;
  }

  // Special case: MSR - Move Register to PSR (TEQ | CMN with SET_CONDITIONS = 0)
  if (!set_conditions && (data_opcode == TEQ || data_opcode == CMN)) {
    decode_move_register_to_psr(cpu, opcode);
    return;
  }

  uint8_t operand_1 = (opcode >> 16) & 0xF;
  uint8_t destination_register = (opcode >> 12) & 0xF;
  uint16_t operand_2 = opcode & 0xFFF;
  bool is_immediate = opcode & IMMEDIATE;

  auto const& operation_funcs = cpu.arm_data_processing_instructions[data_opcode];
  if (!is_immediate && !set_conditions) {
    operation_funcs[REGISTER_AND_NO_CSPR](cpu, data_opcode, operand_1, operand_2, destination_register);
  } else if (is_immediate && !set_conditions) {
    operation_funcs[IMMEDIATE_AND_NO_CSPR](cpu, data_opcode, operand_1, operand_2, destination_register);
  } else if (!is_immediate && set_conditions) {
    operation_funcs[REGISTER_AND_CSPR](cpu, data_opcode, operand_1, operand_2, destination_register);
  } else if (is_immediate && set_conditions) {
    operation_funcs[IMMEDIATE_AND_CSPR](cpu, data_opcode, operand_1, operand_2, destination_register);
  }
}

void prepare_data_processing(CPU& cpu) {
  // Initialize the Data Processing instruction set
  // TODO: Refactor this, just a remnant of the initial implementation.
  cpu.arm_data_processing_instructions[AND] = { EXPAND_PERMUTATIONS(and_op) };
  cpu.arm_data_processing_instructions[EOR] = { EXPAND_PERMUTATIONS(exclusive_or_op) };
  cpu.arm_data_processing_instructions[SUB] = { EXPAND_PERMUTATIONS(subtract_op) };
  cpu.arm_data_processing_instructions[RSB] = { EXPAND_PERMUTATIONS(reverse_subtract_op) };
  cpu.arm_data_processing_instructions[ADD] = { EXPAND_PERMUTATIONS(add_op) };
  cpu.arm_data_processing_instructions[ADC] = { EXPAND_PERMUTATIONS(add_with_carry_op) };
  cpu.arm_data_processing_instructions[SBC] = { EXPAND_PERMUTATIONS(subtract_with_carry_op) };
  cpu.arm_data_processing_instructions[RSC] = { EXPAND_PERMUTATIONS(reverse_subtract_with_carry_op) };
  cpu.arm_data_processing_instructions[TST] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_op) };
  cpu.arm_data_processing_instructions[TEQ] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_exclusive_or_op) };
  cpu.arm_data_processing_instructions[CMP] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(compare_op) };
  cpu.arm_data_processing_instructions[CMN] = { EXPAND_PERMUTATIONS_IGNORE_CSPR(test_add_op) };
  cpu.arm_data_processing_instructions[ORR] = { EXPAND_PERMUTATIONS(or_operation) };
  cpu.arm_data_processing_instructions[MOV] = { EXPAND_PERMUTATIONS(move_op) };
  cpu.arm_data_processing_instructions[BIC] = { EXPAND_PERMUTATIONS(bit_clear_op) };
  cpu.arm_data_processing_instructions[MVN] = { EXPAND_PERMUTATIONS(move_not_op) };
}

// =================================================================================================
// ARM - Multiply
// =================================================================================================

void multiply_op(
  CPU& cpu,
  uint8_t destination_register,
  uint8_t reg_operand_1,
  uint8_t reg_operand_2,
  uint8_t accum_reg,
  bool set_flags,
  bool accumulate
) {
  uint32_t result = cpu.get_register_value(reg_operand_1) * cpu.get_register_value(reg_operand_2);
  if (accumulate) {
    result += cpu.get_register_value(accum_reg);
  }
  cpu.set_register_value(destination_register, result);

  if (set_flags) {
    update_negative_and_zero_cspr_flags(cpu, cpu.get_register_value(destination_register));
  }
}

void multiply_long_op(
  CPU& cpu,
  uint8_t destination_register_low,
  uint8_t destination_register_high,
  uint8_t reg_operand_1,
  uint8_t reg_operand_2,
  bool set_flags,
  bool accumulate
) {
  uint64_t result = (uint64_t)cpu.get_register_value(reg_operand_1) * (uint64_t)cpu.get_register_value(reg_operand_2);
  if (accumulate) {
    uint64_t accumulator = (uint64_t)cpu.get_register_value(destination_register_high) << 32 | cpu.get_register_value(destination_register_low);
    result += accumulator;
  }

  cpu.set_register_value(destination_register_low, result & 0xFFFFFFFF);
  cpu.set_register_value(destination_register_high, result >> 32);

  if (set_flags) {
    update_negative_and_zero_cspr_flags<uint64_t, int64_t>(cpu, result);
  }
}

void multiply_long_signed_op(
  CPU& cpu,
  uint8_t destination_register_low,
  uint8_t destination_register_high,
  uint8_t reg_operand_1,
  uint8_t reg_operand_2,
  bool set_flags,
  bool accumulate
) {
  int64_t result = (int64_t)(int32_t)cpu.get_register_value(reg_operand_1) * (int64_t)(int32_t)cpu.get_register_value(reg_operand_2);
  if (accumulate) {
    int64_t accumulator = (int64_t)(int32_t)cpu.get_register_value(destination_register_high) << 32 | cpu.get_register_value(destination_register_low);
    result += accumulator;
  }

  cpu.set_register_value(destination_register_low, result & 0xFFFFFFFF);
  cpu.set_register_value(destination_register_high, result >> 32);

  if (set_flags) {
    update_negative_and_zero_cspr_flags<int64_t, int64_t>(cpu, result);
  }
}

void decode_multiply(CPU& cpu, uint32_t opcode) {
  uint8_t destination_register = (opcode >> 16) & 0xF;
  uint8_t operand_1_register = (opcode >> 8) & 0xF;
  uint8_t operand_2_register = opcode & 0xF;
  bool accumulate = opcode & (1 << 21);
  bool set_conditions = opcode & SET_CONDITIONS;
  uint8_t accum_register = (opcode >> 12) & 0xF;

  if (destination_register == PC) {
    throw std::runtime_error("PC register cannot be the destination register");
  }

  if (operand_1_register == PC || operand_2_register == PC || accum_register == PC) {
    throw std::runtime_error("PC register cannot be an operand");
  }

  if (operand_1_register == destination_register) {
    throw std::runtime_error("Operand 1 register cannot be the destination register");
  }

  multiply_op(
    cpu,
    destination_register,
    operand_1_register,
    operand_2_register,
    accum_register,
    set_conditions,
    accumulate
  );

  // Increment the PC to the next instruction
  cpu.increment_pc();
}

void decode_multiply_long(CPU& cpu, uint32_t opcode) {
  uint8_t destination_register_low = (opcode >> 12) & 0xF;
  uint8_t destination_register_high = (opcode >> 16) & 0xF;
  uint8_t operand_1_register = (opcode >> 8) & 0xF;
  uint8_t operand_2_register = opcode & 0xF;
  bool accumulate = opcode & (1 << 21);
  bool set_conditions = opcode & SET_CONDITIONS;
  bool signed_multiply = opcode & (1 << 22);

  if (destination_register_high == PC || destination_register_low == PC) {
    throw std::runtime_error("PC register cannot be the destination register");
  }

  if (operand_1_register == PC || operand_2_register == PC) {
    throw std::runtime_error("PC register cannot be an operand");
  }

  if (operand_2_register == destination_register_low || operand_2_register == destination_register_high) {
    throw std::runtime_error("Operand 1 register cannot be the destination register");
  }

  if (destination_register_high == destination_register_low) {
    throw std::runtime_error("Destination register high and low cannot be the same");
  }

  if (signed_multiply) {
    multiply_long_signed_op(
      cpu,
      destination_register_low,
      destination_register_high,
      operand_1_register,
      operand_2_register,
      set_conditions,
      accumulate
    );
  } else {
    multiply_long_op(
      cpu,
      destination_register_low,
      destination_register_high,
      operand_1_register,
      operand_2_register,
      set_conditions,
      accumulate
    );
  }

  // Increment the PC to the next instruction
  cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
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

void store_op(CPU& cpu, uint8_t base_register, uint8_t source_register, uint16_t offset, uint8_t control_flags, bool increment_pc = true) {
  uint32_t base_address = cpu.get_register_value(base_register);
  if (base_register == PC) {
    // If PC is the base register, it should be 8 bytes ahead.
    base_address += 2 * cpu.get_instruction_size();
  }

  uint32_t value = cpu.get_register_value(source_register);
  if (source_register == PC) {
    // If PC is the source register, it should be 12 bytes ahead.
    value += 3 * cpu.get_instruction_size();
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
  bool is_writing_back = (control_flags & WRITE_BACK) || !is_pre_transfer;
  if (is_writing_back) {
    if (base_register == PC) {
      throw std::runtime_error("Cannot write back to PC");
    }
    cpu.set_register_value(base_register, base_address);
  }

  // Increment the PC to the next instruction
  if (increment_pc) cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
}

void load_op(CPU& cpu, uint8_t base_register, uint8_t destination_register, uint16_t offset, uint8_t control_flags, bool increment_pc = true) {
  uint32_t base_address = cpu.get_register_value(base_register);
  uint32_t full_offset = 0;
  bool is_pre_transfer = control_flags & PRE_TRANSFER;

  if (base_register == PC) {
    // If PC is the base register, it should be 8 bytes ahead.
    base_address += 2 * cpu.get_instruction_size();
  }

  if (control_flags & REGISTER_OFFSET) {
    full_offset = apply_shift_operation(cpu, offset);
  } else {
    full_offset = (uint32_t)offset;
  }

  if (is_pre_transfer) {
    base_address += (control_flags & UP) ? full_offset : -full_offset;
  }

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
  bool is_writing_back = (control_flags & WRITE_BACK) || !is_pre_transfer;
  if (is_writing_back) {
    if (base_register == PC) {
      throw std::runtime_error("Cannot write back to PC");
    }
    cpu.set_register_value(base_register, base_address);
  }

  // Increment the PC to the next instruction
  if (increment_pc) cpu.increment_pc();
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
    base_address += 2 * cpu.get_instruction_size();
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
  bool word_aligned = (base_address & 3) == 0;
  bool halfword_aligned = (base_address & 1) == 0;

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

    // Don't increment the PC if the base register is the PC and the write back is enabled.
    if (base_register == PC) return;
  }

  // Increment the PC to the next instruction
  cpu.increment_pc();
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
    base_address += 2 * cpu.get_instruction_size();
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

    // Don't increment the PC if the base register is the PC and the write back is enabled.
    if (base_register == PC) return;
  }

  // Increment the PC to the next instruction
  cpu.increment_pc();
}

void decode_load_and_store(CPU& cpu, uint32_t opcode) {
  uint8_t base_register = (opcode >> 16) & 0xF;
  uint8_t destination_register = (opcode >> 12) & 0xF;
  uint16_t offset = opcode & 0xFFF;
  uint8_t control_flags = (opcode >> 21) & 0x1F;
  bool is_load = opcode & (1 << 20);

  if (is_load) {
    load_op(cpu, base_register, destination_register, offset, control_flags);
  } else {
    store_op(cpu, base_register, destination_register, offset, control_flags);
  }
}

void decode_half_word_load_and_store(CPU& cpu, uint32_t opcode) {
  uint8_t base_register = (opcode >> 16) & 0xF;
  uint8_t destination_register = (opcode >> 12) & 0xF;
  bool is_load = opcode & (1 << 20);
  bool is_immediate = opcode & (1 << 22);

  // Control flags = (P) Pre/Post | (U) Up/Down | (W) WriteBack | (S) Signed | (H) Halfword (5 bits)
  uint8_t control_flags = ((opcode >> 23) & 3) << 3; // PU
  control_flags |= ((opcode >> 21) & 1) << 2; // W
  control_flags |= (opcode >> 5) & 3; // SH

  // TODO: Refactor this, no need for the templated functions anymore.
  if (is_immediate) {
    uint8_t immediate_value = opcode & 0xF | ((opcode >> 4) & 0xF0);
    if (is_load) {
      load_halfword_signed_byte<Immediate>(cpu, base_register, destination_register, immediate_value, control_flags);
    } else {
      store_halfword_signed_byte<Immediate>(cpu, base_register, destination_register, immediate_value, control_flags);
    }
    return;
  }

  uint8_t offset_register = opcode & 0xF;
  if (is_load) {
    load_halfword_signed_byte(cpu, base_register, destination_register, (uint16_t)offset_register, control_flags);
  } else {
    store_halfword_signed_byte(cpu, base_register, destination_register, (uint16_t)offset_register, control_flags);
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
    if ((register_list & (1 << register_idx)) == 0) continue;

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

  // Increment the PC to the next instruction
  if (!pc_in_list) cpu.increment_pc();
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
    if ((register_list & (1 << register_idx)) == 0) continue;

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

  // Increment the PC to the next instruction
  cpu.increment_pc();
}

void decode_block_data_transfer(CPU& cpu, uint32_t opcode) {
  uint8_t base_register = (opcode >> 16) & 0xF;
  uint16_t register_list = opcode & 0xFFFF;
  bool is_load = opcode & (1 << 20);

  // Control flags = (P) Pre/Post - (U) Up/Down - (S) PSR / Force User Bit - (W) Write Back (4 bits)
  uint8_t control_flags = (opcode >> 21) & 0xF;

  if (is_load) {
    block_load(cpu, base_register, register_list, control_flags);
  } else {
    block_store(cpu, base_register, register_list, control_flags);
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
    load_op(cpu, base_register, destination_register, 0, BYTE_QUANTITY, false /* skip PC increment */);
    store_op(cpu, base_register, source_register, 0, BYTE_QUANTITY, false /* skip PC increment */);
  } else {
    load_op(cpu, base_register, destination_register, 0, 0, false /* skip PC increment */);
    store_op(cpu, base_register, source_register, 0, 0, false /* skip PC increment */);
  }
}

void decode_single_data_swap(CPU& cpu, uint32_t opcode) {
  uint8_t base_register = (opcode >> 16) & 0xF;
  uint8_t destination_register = (opcode >> 12) & 0xF;
  uint8_t source_register = opcode & 0xF;
  bool is_byte_swap = opcode & (1 << 22);

  if (is_byte_swap) {
    single_data_swap<SwapByte>(cpu, base_register, destination_register, source_register);
  } else {
    single_data_swap<SwapWord>(cpu, base_register, destination_register, source_register);
  }
}

// =================================================================================================
// ARM - Software Interrupt (SWI)
// =================================================================================================

void software_interrupt(CPU& cpu) {
  // Backup the current PC and CPSR
  uint32_t current_pc = cpu.get_register_value(PC);
  uint32_t current_cspr = cpu.cspr;

  // Switch to Supervisor mode & back to ARM state.
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

// =================================================================================================
// THUMB - Move Shifted Register
// =================================================================================================

static constexpr uint32_t ARM_MOVS_REGISTER_OFFSET_IMMEDIATE_SHIFT_OPCODE = MOV << 21 | SET_CONDITIONS;

void decode_thumb_move_shifted_register(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t source_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset_immediate = (instruction >> 6) & 0x1F; // Next 5 bits
  uint8_t operation = (instruction >> 11) & 0x3; // Next 2 bits

  // TODO: Optimize this to avoid more shifting in the `decode_data_processing` function.
  // Convert the THUMB instruction to an ARM equivalent
  // movs destination_register, source_register, #offset_immediate
  uint32_t arm_equiv_instruction = ARM_MOVS_REGISTER_OFFSET_IMMEDIATE_SHIFT_OPCODE |
    (destination_register << 12) |
    (offset_immediate << 7) |
    (operation << 5) |
    source_register;
  decode_data_processing(cpu, arm_equiv_instruction);
}

// =================================================================================================
// THUMB - Add / Subtract
// =================================================================================================

static constexpr uint32_t ARM_ADD_REGISTER_OPCODE = ADD << 21 | SET_CONDITIONS;
static constexpr uint32_t ARM_ADD_REGISTER_IMMEDIATE_OFFSET_OPCODE = ARM_ADD_REGISTER_OPCODE | IMMEDIATE;
static constexpr uint32_t ARM_SUB_REGISTER_OPCODE = SUB << 21 | SET_CONDITIONS;
static constexpr uint32_t ARM_SUB_REGISTER_IMMEDIATE_OFFSET_OPCODE = ARM_SUB_REGISTER_OPCODE | IMMEDIATE;

void decode_thumb_add_sub(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t source_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset = (instruction >> 6) & 0x7; // Next 3 bits
  uint32_t input_component = (source_register << 16) | (destination_register << 12) | offset;

  // Op (0 = Add, 1 = Sub) & Immediate (0 = Register, 1 = Immediate)
  uint8_t operation = (instruction >> 9) & 0x3;

  // TODO: Optimize this to avoid more shifting in the `decode_data_processing` function.
  // Convert the THUMB instruction to an ARM equivalent
  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0b00:
      // adds rd, rs, rn
      arm_instruction = ARM_ADD_REGISTER_OPCODE | input_component;
      break;
    case 0b01:
      // subs rd, rs, rn
      arm_instruction = ARM_SUB_REGISTER_OPCODE | input_component;
      break;
    case 0b10:
      // adds rd, rs, #offset
      arm_instruction = ARM_ADD_REGISTER_IMMEDIATE_OFFSET_OPCODE | input_component;
      break;
    case 0b11:
      // subs rd, rs, #offset
      arm_instruction = ARM_SUB_REGISTER_IMMEDIATE_OFFSET_OPCODE | input_component;
      break;
  }
  decode_data_processing(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - MOV/CMP/ADD/SUB (Immediate)
// =================================================================================================

static constexpr uint32_t ARM_MOV_IMMEDIATE_OPCODE = MOV << 21 | SET_CONDITIONS | IMMEDIATE;
static constexpr uint32_t ARM_CMP_IMMEDIATE_OPCODE = CMP << 21 | SET_CONDITIONS | IMMEDIATE;
static constexpr uint32_t ARM_ADD_IMMEDIATE_OPCODE = ADD << 21 | SET_CONDITIONS | IMMEDIATE;
static constexpr uint32_t ARM_SUB_IMMEDIATE_OPCODE = SUB << 21 | SET_CONDITIONS | IMMEDIATE;

void decode_thumb_mov_cmp_add_sub_immediate(CPU& cpu, uint16_t instruction) {
  uint8_t offset = instruction & 0xFF; // Last 8 bits
  uint8_t destination_register = (instruction >> 8) & 0x7; // Next 3 bits
  uint8_t operation = (instruction >> 11) & 0x3; // Next 2 bits
  uint32_t input_component = (destination_register << 12) | offset;

  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0:
      // movs destination_register, #offset
      arm_instruction = ARM_MOV_IMMEDIATE_OPCODE | input_component;
      break;
    case 1:
      // cmp destination_register, #offset
      arm_instruction = ARM_CMP_IMMEDIATE_OPCODE | input_component;
      break;
    case 2:
      // adds destination_register, destination_register, #offset
      arm_instruction = ARM_ADD_IMMEDIATE_OPCODE | (destination_register << 16) | input_component;
      break;
    case 3:
      // subs destination_register, destination_register, #offset
      arm_instruction = ARM_SUB_IMMEDIATE_OPCODE | (destination_register << 16) | input_component;
      break;
  }

  decode_data_processing(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - ALU Operations
// =================================================================================================

static constexpr uint32_t ARM_AND_REGISTER_OPCODE = (AND << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_EOR_REGISTER_OPCODE = (EOR << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_MOV_REGISTER_OPCODE = (MOV << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_ADC_REGISTER_OPCODE = (ADC << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_SBC_REGISTER_OPCODE = (SBC << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_TST_REGISTER_OPCODE = (TST << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_RSB_IMMEDIATE_OPCODE = (RSB << 21) | SET_CONDITIONS | IMMEDIATE;
static constexpr uint32_t ARM_CMP_REGISTER_OPCODE = (CMP << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_CMN_REGISTER_OPCODE = (CMN << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_ORR_REGISTER_OPCODE = (ORR << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_MUL_REGISTER_OPCODE = (1 << 7) | (1 << 4) | SET_CONDITIONS;
static constexpr uint32_t ARM_BIC_REGISTER_OPCODE = (BIC << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_MVN_REGISTER_OPCODE = (MVN << 21) | SET_CONDITIONS;
static constexpr uint32_t ARM_SHIFT_BY_REGISTER_FLAG = 1 << 4;
static constexpr uint32_t ARM_LSR_COMPONENT = 1 << 5;
static constexpr uint32_t ARM_ASR_COMPONENT = 2 << 5;
static constexpr uint32_t ARM_ROR_COMPONENT = 3 << 5;

void decode_thumb_alu_operations(CPU& cpu, uint32_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t source_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t operation = (instruction >> 6) & 0xF; // Next 4 bits
  uint32_t rd_rd_component = (destination_register << 16) | (destination_register << 12);

  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0:
      // ands rd, rd, rs
      arm_instruction = ARM_AND_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 1:
      // eors rd, rd, rs
      arm_instruction = ARM_EOR_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 2:
      // movs rd, rd, lsl rs
      arm_instruction = ARM_MOV_REGISTER_OPCODE | rd_rd_component | ARM_SHIFT_BY_REGISTER_FLAG | (source_register << 8);
      break;
    case 3:
      // movs rd, rd, lsr rs
      arm_instruction = ARM_MOV_REGISTER_OPCODE | rd_rd_component | ARM_SHIFT_BY_REGISTER_FLAG | ARM_LSR_COMPONENT | (source_register << 8);
      break;
    case 4:
      // movs rd, rd, asr rs
      arm_instruction = ARM_MOV_REGISTER_OPCODE | rd_rd_component | ARM_SHIFT_BY_REGISTER_FLAG | ARM_ASR_COMPONENT | (source_register << 8);
      break;
    case 5:
      // adcs rd, rd, rs
      arm_instruction = ARM_ADC_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 6:
      // sbcs rd, rd, rs
      arm_instruction = ARM_SBC_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 7:
      // movs rd, rd, ror rs
      arm_instruction = ARM_MOV_REGISTER_OPCODE | rd_rd_component | ARM_SHIFT_BY_REGISTER_FLAG | ARM_ROR_COMPONENT | (source_register << 8);
      break;
    case 8:
      // tst rd, rs
      arm_instruction = ARM_TST_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 9:
      // rsbs rd, rs, #0
      arm_instruction = ARM_RSB_IMMEDIATE_OPCODE | (source_register << 16) | (destination_register << 12);
      break;
    case 10:
      // cmp rd, rs
      arm_instruction = ARM_CMP_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 11:
      // cmn rd, rs
      arm_instruction = ARM_CMN_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 12:
      // orrs rd, rd, rs
      arm_instruction = ARM_ORR_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 13:
      // muls rd, rs, rd
      arm_instruction = ARM_MUL_REGISTER_OPCODE | (destination_register << 16) | (source_register << 8) | destination_register;
      break;
    case 14:
      // bics rd, rd, rs
      arm_instruction = ARM_BIC_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
    case 15:
      // mvns rd, rs
      arm_instruction = ARM_MVN_REGISTER_OPCODE | rd_rd_component | source_register;
      break;
  }
  if (operation == 13) {
    decode_multiply(cpu, arm_instruction);
  } else {
    decode_data_processing(cpu, arm_instruction);
  }
}

// =================================================================================================
// THUMB - Hi Register Operations / Branch Exchange
// =================================================================================================

static constexpr uint32_t ARM_ADD_REGISTER_NO_SET_COND_OPCODE = ADD << 21;
static constexpr uint32_t ARM_MOV_REGISTER_NO_SET_COND_OPCODE = MOV << 21;
static constexpr uint32_t ARM_BX_OPCODE = 0b0000'0001'0010'1111'1111'1111'0001'0000;

void decode_thumb_hi_register_operations_branch_exchange(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t source_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t operation = (instruction >> 6) & 0xF; // Next 4 bits (Op, H1, H2)

  uint32_t arm_instruction = 0;
  switch (operation) {
    case 1:
      // add rd, rd, hs
      arm_instruction = ARM_ADD_REGISTER_NO_SET_COND_OPCODE | (destination_register << 12) | (destination_register << 16) | (source_register + 8);
      break;
    case 2:
      // add hd, hd, rs
      arm_instruction = ARM_ADD_REGISTER_NO_SET_COND_OPCODE | ((destination_register + 8) << 12) | ((destination_register + 8) << 16) | source_register;
      break;
    case 3:
      // add hd, hd, hs
      arm_instruction = ARM_ADD_REGISTER_NO_SET_COND_OPCODE | ((destination_register + 8) << 12) | ((destination_register + 8) << 16) | (source_register + 8);
      break;
    case 5:
      // cmp rd, hs
      arm_instruction = ARM_CMP_REGISTER_OPCODE | (destination_register << 16) | (source_register + 8);
      break;
    case 6:
      // cmp hd, rs
      arm_instruction = ARM_CMP_REGISTER_OPCODE | ((destination_register + 8) << 16) | source_register;
      break;
    case 9:
      // mov rd, hs
      arm_instruction = ARM_MOV_REGISTER_NO_SET_COND_OPCODE | (destination_register << 12) | (source_register + 8);
      break;
    case 10:
      // mov hd, rs
      arm_instruction = ARM_MOV_REGISTER_NO_SET_COND_OPCODE | ((destination_register + 8) << 12) | source_register;
      break;
    case 11:
      // mov hd, hs
      arm_instruction = ARM_MOV_REGISTER_NO_SET_COND_OPCODE | ((destination_register + 8) << 12) | (source_register + 8);
      break;
    case 12:
      // bx rs
      arm_instruction = ARM_BX_OPCODE | source_register;
      break;
    case 13:
      // bx hs
      arm_instruction = ARM_BX_OPCODE | (source_register + 8);
      break;
  }

  if (operation > 11) {
    decode_branch_and_exchange(cpu, instruction);
  } else {
    decode_data_processing(cpu, arm_instruction);
  }
}

// =================================================================================================
// THUMB - PC-relative load
// =================================================================================================

// ldr rd, [pc, #immediate]
static constexpr uint32_t ARM_LDR_PC_RELATIVE_OPCODE = (1 << 26) | (1 << 24) | (1 << 23) | (1 << 20) | (15 << 16);

void decode_thumb_pc_relative_load(CPU& cpu, uint16_t instruction) {
  uint32_t immediate_value = (instruction & 0xFF) << 2; // Last 8 bits, shifted by 2 to become a 10-bit value
  uint8_t destination_register = (instruction >> 8) & 0x7; // Next 3 bits
  uint32_t arm_instruction = ARM_LDR_PC_RELATIVE_OPCODE | (destination_register << 12) | immediate_value;
  decode_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Load/Store w/ Register Offset
// =================================================================================================

static constexpr uint32_t ARM_STR_REGISTER_OFFSET_OPCODE = (1 << 26) | (1 << 25) | (1 << 24) | (1 << 23);
static constexpr uint32_t ARM_LDR_REGISTER_OFFSET_OPCODE = ARM_STR_REGISTER_OFFSET_OPCODE | (1 << 20);
static constexpr uint32_t ARM_BYTE_QUANTITY_FLAG = 1 << 22;

void decode_thumb_load_store_register_offset(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t base_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset_register = (instruction >> 6) & 0x7; // Next 3 bits
  uint8_t operation = (instruction >> 10) & 0x3; // Skip 1 bit, and get the next 2 bits
  uint32_t input_component = (destination_register << 12) | (base_register << 16) | offset_register;
  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0:
      // str rd, [rb, ro]
      arm_instruction = ARM_STR_REGISTER_OFFSET_OPCODE | input_component;
      break;
    case 1:
      // strb rd, [rb, ro]
      arm_instruction = ARM_STR_REGISTER_OFFSET_OPCODE | ARM_BYTE_QUANTITY_FLAG | input_component;
      break;
    case 2:
      // ldr rd, [rb, ro]
      arm_instruction = ARM_LDR_REGISTER_OFFSET_OPCODE | input_component;
      break;
    case 3:
      // ldrb rd, [rb, ro]
      arm_instruction = ARM_LDR_REGISTER_OFFSET_OPCODE | ARM_BYTE_QUANTITY_FLAG | input_component;
      break;
  }
  decode_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Load/Store Sign-Extended Byte/Halfword
// =================================================================================================

static constexpr uint32_t ARM_BASE_HALFWORD_OR_SIGNED_STR_REGISTER_OFFSET_OPCODE = (1 << 24) | (1 << 23) | (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_BASE_HALFWORD_OR_SIGNED_LDR_REGISTER_OFFSET_OPCODE = ARM_BASE_HALFWORD_OR_SIGNED_STR_REGISTER_OFFSET_OPCODE | (1 << 20);
static constexpr uint32_t ARM_HALFWORD_FLAG = 1 << 5;
static constexpr uint32_t ARM_SIGNED_FLAG = 1 << 6;

void decode_thumb_load_store_sign_extended_byte_halfword(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t base_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset_register = (instruction >> 6) & 0x7; // Next 3 bits
  uint8_t operation = (instruction >> 10) & 0x3; // Skip 1 bit, and get the next 2 bits
  uint32_t input_component = (destination_register << 12) | (base_register << 16) | offset_register;

  uint32_t arm_instruction = 0;
  switch(operation) {
    case 0:
      // strh rd, [rb, ro]
      arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_STR_REGISTER_OFFSET_OPCODE | ARM_HALFWORD_FLAG | input_component;
      break;
    case 1:
      // ldrh rd, [rb, ro]
      arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_LDR_REGISTER_OFFSET_OPCODE | ARM_HALFWORD_FLAG | input_component;
      break;
    case 2:
      // ldsb rd, [rb, ro]
      arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_LDR_REGISTER_OFFSET_OPCODE | ARM_SIGNED_FLAG | input_component;
      break;
    case 3: 
      // ldsh rd, [rb, ro]
      arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_LDR_REGISTER_OFFSET_OPCODE | ARM_HALFWORD_FLAG | ARM_SIGNED_FLAG | input_component;
      break;
  }
  decode_half_word_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Load/Store w/ Immediate Offset
// =================================================================================================

static constexpr uint32_t ARM_STR_IMMEDIATE_OFFSET_OPCODE = (1 << 26) | (1 << 24) | (1 << 23);
static constexpr uint32_t ARM_LDR_IMMEDIATE_OFFSET_OPCODE = ARM_STR_IMMEDIATE_OFFSET_OPCODE | (1 << 20);

void decode_thumb_load_store_immediate_offset(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t base_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset = (instruction >> 6) & 0x1F; // Next 5 bits
  uint8_t operation = (instruction >> 11) & 0x3; // Next 2 bits
  uint32_t input_component = (destination_register << 12) | (base_register << 16);

  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0:
      // str rd, [rb, #offset]
      offset <<= 2; // Shift the offset by 2 to make it a 7-bit value.
      arm_instruction = ARM_STR_IMMEDIATE_OFFSET_OPCODE | input_component | offset;
      break;
    case 1:
      // strb rd, [rb, #offset]
      arm_instruction = ARM_STR_IMMEDIATE_OFFSET_OPCODE | ARM_BYTE_QUANTITY_FLAG | input_component | offset;
      break;
    case 2:
      // ldr rd, [rb, #offset]
      offset <<= 2; // Shift the offset by 2 to make it a 7-bit value.
      arm_instruction = ARM_LDR_IMMEDIATE_OFFSET_OPCODE | input_component | offset;
      break;
    case 3:
      // ldrb rd, [rb, #offset]
      arm_instruction = ARM_LDR_IMMEDIATE_OFFSET_OPCODE | ARM_BYTE_QUANTITY_FLAG | input_component | offset;
      break;
  }
  decode_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Load/Store Halfword
// =================================================================================================

static constexpr uint32_t ARM_BASE_HALFWORD_OR_SIGNED_STR_IMMEDIATE_OFFSET_OPCODE = (1 << 24) | (1 << 23) | (1 << 22) | (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_BASE_HALFWORD_OR_SIGNED_LDR_IMMEDIATE_OFFSET_OPCODE = ARM_BASE_HALFWORD_OR_SIGNED_STR_REGISTER_OFFSET_OPCODE | (1 << 20);

void decode_thumb_load_store_halfword(CPU& cpu, uint16_t instruction) {
  uint8_t destination_register = instruction & 0x7; // Last 3 bits
  uint8_t base_register = (instruction >> 3) & 0x7; // Next 3 bits
  uint8_t offset = ((instruction >> 6) & 0x1F) << 1; // Next 5 bits, shifted by 1 to make it a 6-bit value
  bool is_load = instruction & (1 << 11);
  uint32_t input_component = (destination_register << 12) | (base_register << 16) | offset;

  uint32_t arm_instruction = 0;
  if (is_load) {
    // ldrh rd, [rb, #offset]
    arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_LDR_IMMEDIATE_OFFSET_OPCODE | ARM_HALFWORD_FLAG | input_component;
  } else {
    // strh rd, [rb, #offset]
    arm_instruction = ARM_BASE_HALFWORD_OR_SIGNED_STR_IMMEDIATE_OFFSET_OPCODE | ARM_HALFWORD_FLAG | input_component;
  }
  decode_half_word_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - SP-relative Load/Store
// =================================================================================================

static constexpr uint32_t ARM_STR_SP_RELATIVE_OPCODE = ARM_STR_IMMEDIATE_OFFSET_OPCODE | (SP << 16);
static constexpr uint32_t ARM_LDR_SP_RELATIVE_OPCODE = ARM_LDR_IMMEDIATE_OFFSET_OPCODE | (SP << 16);

void decode_thumb_sp_relative_load_store(CPU& cpu, uint16_t instruction) {
  uint16_t immediate_offset = (instruction & 0xFF) << 2; // Last 8 bits, shifted by 2 to make it a 10-bit value
  uint8_t destination_register = (instruction >> 8) & 0x7; // Next 3 bits
  bool is_load = instruction & (1 << 11);
  uint32_t input_component = (destination_register << 12) | immediate_offset;

  uint32_t arm_instruction = 0;
  if (is_load) {
    // ldr rd, [sp, #immediate_offset]
    arm_instruction = ARM_LDR_IMMEDIATE_OFFSET_OPCODE | input_component;
  } else {
    // str rd, [sp, #immediate_offset]
    arm_instruction = ARM_STR_IMMEDIATE_OFFSET_OPCODE | input_component;
  }
  decode_load_and_store(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Load Address
// =================================================================================================

static constexpr uint32_t ARM_ADD_IMMEDIATE_NO_SET_COND_OPCODE = ADD << 21 | IMMEDIATE;
static constexpr uint32_t ARM_ADD_TO_PC_OPCODE = ARM_ADD_IMMEDIATE_NO_SET_COND_OPCODE | (PC << 16);
static constexpr uint32_t ARM_ADD_TO_SP_OPCODE = ARM_ADD_IMMEDIATE_NO_SET_COND_OPCODE | (SP << 16);

void decode_thumb_load_address(CPU& cpu, uint16_t instruction) {
  uint8_t immediate_value = (instruction & 0xFF) << 2; // Last 8 bits, shifted by 2 to make it a 10-bit value
  uint8_t destination_register = (instruction >> 8) & 0x7; // Next 3 bits
  bool is_sp = instruction & (1 << 11);
  uint32_t input_component = (destination_register << 12) | immediate_value;

  uint32_t arm_instruction = 0;
  if (is_sp) {
    // add rd, sp, #immediate_value
    arm_instruction = ARM_ADD_TO_SP_OPCODE | input_component;
  } else {
    // add rd, pc, #immediate_value
    arm_instruction = ARM_ADD_TO_PC_OPCODE | input_component;
  }
  decode_data_processing(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Add offset to SP
// =================================================================================================

static constexpr uint32_t ARM_ADD_TO_SP_AND_STORE_IN_SP_OPCODE = ARM_ADD_TO_PC_OPCODE | (SP << 12);
static constexpr uint32_t ARM_SUB_FROM_SP_AND_STORE_IN_SP_OPCODE = (SUB << 21) | IMMEDIATE | (SP << 16) | (SP << 12);

void decode_thumb_add_offset_to_stack_pointer(CPU& cpu, uint16_t instruction) {
  uint8_t magnitude_value = (instruction & 0x7F) << 2; // Last 7 bits, shifted by 2 to make it a 9-bit value
  bool is_subtract = instruction & (1 << 7);

  uint32_t arm_instruction = 0;
  if (is_subtract) {
    // sub sp, sp, #magnitude_value
    arm_instruction = ARM_SUB_FROM_SP_AND_STORE_IN_SP_OPCODE | magnitude_value;
  } else {
    // add sp, sp, #magnitude_value
    arm_instruction = ARM_ADD_TO_SP_AND_STORE_IN_SP_OPCODE | magnitude_value;
  }
  decode_data_processing(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Push/Pop Registers
// =================================================================================================

static constexpr uint32_t ARM_STM_TO_SP_OPCODE = (1 << 27) | (1 << 24) | (1 << 21) | (SP << 16);
static constexpr uint32_t ARM_LDM_TO_SP_OPCODE = ARM_STM_TO_SP_OPCODE | (1 << 20);

void decode_thumb_push_pop_registers(CPU& cpu, uint16_t instruction) {
  uint16_t register_list = (instruction & 0xFF) << 8; // Last 8 bits
  uint8_t operation = ((instruction >> 8) & 1) | ((instruction >> 11) & 1); // Next bit (PC/LR bit), skip 2 bits, and get the next bit (Load/Store bit).

  uint32_t arm_instruction = 0;
  switch (operation) {
    case 0:
      // stmdb sp!, {rlist}
      arm_instruction = ARM_STM_TO_SP_OPCODE | register_list;
      break;
    case 1:
      // stmdb sp!, {rlist, lr}
      arm_instruction = ARM_STM_TO_SP_OPCODE | (1 << LR) | register_list;
      break;
    case 2:
      // ldmia sp!, {rlist}
      arm_instruction = ARM_LDM_TO_SP_OPCODE | register_list;
      break;
    case 3:
      // ldmia sp!, {rlist, pc}
      arm_instruction = ARM_LDM_TO_SP_OPCODE | (1 << PC) | register_list;
      break;
  }
  decode_block_data_transfer(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Multiple Load/Store
// =================================================================================================

static constexpr uint32_t ARM_STM_OPCODE = (1 << 27) | (1 << 21);
static constexpr uint32_t ARM_LDM_OPCODE = ARM_STM_OPCODE | (1 << 20);

void decode_thumb_multiple_load_store(CPU& cpu, uint16_t instruction) {
  uint16_t register_list = (instruction & 0xFF) << 8; // Last 8 bits, shifted by 8 to make it a 16-bit value
  uint8_t base_register = (instruction >> 8) & 0x7; // Next 3 bits
  bool is_load = instruction & (1 << 11); // Next bit
  uint32_t input_component = (base_register << 16) | register_list;

  uint32_t arm_instruction = 0;
  if (is_load) {
    // ldmia rb!, {rlist}
    arm_instruction = ARM_LDM_OPCODE | input_component;
  } else {
    // stmia rb!, {rlist}
    arm_instruction = ARM_STM_OPCODE | input_component;
  }
  decode_block_data_transfer(cpu, arm_instruction);
}

// =================================================================================================
// THUMB - Conditional Branch
// =================================================================================================

void decode_thumb_conditional_branch(CPU& cpu, uint16_t instruction) {
  uint16_t signed_offset = (instruction & 0xFF) << 1; // Last 8 bits, shifted by 1 to make it a 9-bit value
  uint8_t condition = (instruction >> 8) & 0xF; // Next 4 bits

  // Sign extend the offset to become a 24-bit value
  uint32_t offset = signed_offset;
  if (offset & 0x100) {
    offset |= 0xFFFFFE00;
  }

  uint32_t arm_instruction = ARM_BRANCH_OPCODE | offset;
  if (evaluate_arm_condition(cpu, condition)) {
    decode_branch_and_link(cpu, arm_instruction);
  }
}

// =================================================================================================
// THUMB - Software Interrupt
// =================================================================================================

void decode_thumb_software_interrupt(CPU& cpu, uint16_t instruction) {
  // swi #immediate
  software_interrupt(cpu);
}

// =================================================================================================
// THUMB - Unconditional Branch
// =================================================================================================

void decode_thumb_unconditional_branch(CPU& cpu, uint16_t instruction) {
  uint16_t signed_offset = (instruction & 0x7FF) << 1; // Last 11 bits, shifted by 1 to make it a 12-bit value
  uint32_t offset = signed_offset;
  if (offset & 0x800) {
    offset |= 0xFFFFF000;
  }
  branch(offset, cpu);
}

// =================================================================================================
// THUMB - Long Branch with Link
// =================================================================================================

void decode_thumb_long_branch_with_link(CPU& cpu, uint16_t instruction) {
  bool is_low_offset = instruction & (1 << 11);
  uint16_t offset = (instruction & 0x7FF); // Last 11 bits
  if (is_low_offset) {
    // LR := PC + OffsetHigh << 12
    uint32_t pc = cpu.get_register_value(PC) + 2 * cpu.get_instruction_size();
    cpu.set_register_value(LR, pc + (offset << 12));

    // Increment the PC.
    cpu.increment_pc();
  } else {
    // Shift the offset by 1 to make it a 12-bit value
    offset <<= 1;

    // Calculate the next instruction address (with bit 0 set to 1 to indicate Thumb mode)
    uint32_t next_instruction_addr = (cpu.get_register_value(PC) + cpu.get_instruction_size()) | 1;
    
    // PC := LR + Offset
    cpu.set_register_value(PC, cpu.get_register_value(LR) + offset);
    
    // LR := Address of the next instruction
    cpu.set_register_value(LR, next_instruction_addr);
  }
}

void cpu_reset(CPU& cpu) {
  // TODO: Reset registers etc.
  // TODO: Set the mode back to ARM.

  // Initialize the PC to 0.
  cpu_arm_write_pc(cpu, 0);
}

void cpu_init(CPU& cpu) {
  // TODO: Some refactoring and we might be able to remove this.
  prepare_data_processing(cpu);

  // Reset the CPU
  cpu_reset(cpu);
}

#define IS(VAL, MASK) (VAL & MASK) == MASK

void execute_thumb_instruction(CPU& cpu, uint32_t instruction) {  
  if (IS(instruction, THUMB_LONG_BRANCH_WITH_LINK_OPCODE)) {
    decode_thumb_long_branch_with_link(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_UNCONDITIONAL_BRANCH_OPCODE)) {
    decode_thumb_unconditional_branch(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_SOFTWARE_INTERRUPT_OPCODE)) {
    decode_thumb_software_interrupt(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_CONDITIONAL_BRANCH_OPCODE)) {
    decode_thumb_conditional_branch(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_MULTIPLE_LOAD_STORE_OPCODE)) {
    decode_thumb_multiple_load_store(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_PUSH_POP_REGISTERS_OPCODE)) {
    decode_thumb_push_pop_registers(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_ADD_OFFSET_TO_STACK_POINTER_OPCODE)) {
    decode_thumb_add_offset_to_stack_pointer(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_LOAD_ADDRESS_OPCODE)) {
    decode_thumb_load_address(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_SP_RELATIVE_LOAD_STORE_OPCODE)) {
    decode_thumb_sp_relative_load_store(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_LOAD_STORE_HALFWORD_OPCODE)) {
    decode_thumb_load_store_halfword(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_LOAD_STORE_IMMEDIATE_OFFSET_OPCODE)) {
    decode_thumb_load_store_immediate_offset(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_LOAD_STORE_SIGN_EXTENDED_BYTE_HALFWORD_OPCODE)) {
    decode_thumb_load_store_sign_extended_byte_halfword(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_LOAD_STORE_REGISTER_OFFSET_OPCODE)) {
    decode_thumb_load_store_register_offset(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_PC_RELATIVE_LOAD_OPCODE)) {
    decode_thumb_pc_relative_load(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_HI_REGISTER_OPERATIONS_BRANCH_EXCHANGE_OPCODE)) {
    decode_thumb_hi_register_operations_branch_exchange(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_ALU_OPERATIONS_OPCODE)) {
    decode_thumb_alu_operations(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_MOV_CMP_ADD_SUB_IMMEDIATE_OPCODE)) {
    decode_thumb_mov_cmp_add_sub_immediate(cpu, instruction);
    return;
  }

  if (IS(instruction, THUMB_ADD_SUB_OPCODE)) {
    decode_thumb_add_sub(cpu, instruction);
    return;
  }

  decode_thumb_move_shifted_register(cpu, instruction);
}

bool evaluate_arm_condition(CPU& cpu, uint8_t condition) {
  switch (condition) {
    case EQ: return (cpu.cspr & CSPR_Z) != 0; // Z == 1
    case NE: return (cpu.cspr & CSPR_Z) == 0; // Z == 0
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

  // Decode the opcode, set the condition code to 0
  // TODO: This might be unnecessary now that we don't have a hashmap for instruction lookup.
  uint32_t opcode = instruction & 0x0FFFFFFF;

  // Check if the instruction is a software interrupt
  if ((opcode & ARM_SOFTWARE_INTERRUPT_OPCODE) == ARM_SOFTWARE_INTERRUPT_OPCODE) {
    software_interrupt(cpu);
    return;
  }

  // Check if the instruction is a coprocessor instruction
  if ((opcode & ARM_COPROCESSOR_OPCODE) == ARM_COPROCESSOR_OPCODE) {
    // Skip coprocessor instructions since the GBA doesn't use them.
    cpu.set_register_value(PC, cpu.get_register_value(PC) + ARM_INSTRUCTION_SIZE);
    return;
  }

  // Check if the instruction is a branch
  if ((opcode & ARM_BRANCH_OPCODE) == ARM_BRANCH_OPCODE) {
    decode_branch_and_link(cpu, opcode);
    return;
  }

  // Check if the instruction is block data transfer
  if ((opcode & ARM_BLOCK_DATA_TRANSFER_OPCODE) == ARM_BLOCK_DATA_TRANSFER_OPCODE) {
    decode_block_data_transfer(cpu, opcode);
    return;
  }

  if ((opcode & ARM_UNDEFINED_OPCODE) == ARM_UNDEFINED_OPCODE) {
    undefined_instruction(cpu);
    return;
  }

  // Check if the instruction is single data transfer
  if ((opcode & ARM_SINGLE_DATA_TRANSFER_OPCODE) == ARM_SINGLE_DATA_TRANSFER_OPCODE) {
    decode_load_and_store(cpu, opcode);
    return;
  }

  // Check if the instruction is a halfword data transfer immediate
  if ((opcode & ARM_HALFWORD_DATA_TRANSFER_IMMEDIATE_OPCODE) == ARM_HALFWORD_DATA_TRANSFER_IMMEDIATE_OPCODE && (opcode & ARM_HALFWORD_DATA_TRANSFER_SH_MASK) > 0) {
    decode_half_word_load_and_store(cpu, opcode);
    return;
  }

  // Check if the instruction is a halfword data transfer register
  if ((opcode & ARM_HALFWORD_DATA_TRANSFER_REGISTER_OPCODE) == ARM_HALFWORD_DATA_TRANSFER_REGISTER_OPCODE && (opcode & ARM_HALFWORD_DATA_TRANSFER_SH_MASK) > 0) {
    decode_half_word_load_and_store(cpu, opcode);
    return;
  }

  // Check if the instruction is branch and exchange
  if ((opcode & ARM_BRANCH_AND_EXCHANGE_OPCODE) == ARM_BRANCH_AND_EXCHANGE_OPCODE) {
    decode_branch_and_exchange(cpu, opcode);
    return;
  }

  if ((opcode & ARM_SINGLE_DATA_SWAP_OPCODE) == ARM_SINGLE_DATA_SWAP_OPCODE) {
    decode_single_data_swap(cpu, opcode);
    return;
  }

  if ((opcode & ARM_MULTIPLY_LONG_OPCODE) == ARM_MULTIPLY_LONG_OPCODE && (opcode & ARM_HALFWORD_DATA_TRANSFER_SH_MASK) == 0) {
    decode_multiply_long(cpu, opcode);
    return;
  }

  if ((opcode & ARM_MULTIPLY_OPCODE) == ARM_MULTIPLY_OPCODE && (opcode & ARM_HALFWORD_DATA_TRANSFER_SH_MASK) == 0) {
    decode_multiply(cpu, opcode);
    return;
  }

  // Otherwise assume it's a data processing instruction
  decode_data_processing(cpu, opcode);
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
