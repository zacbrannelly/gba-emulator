#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include "ram.h"

static constexpr uint8_t ARM_INSTRUCTION_SIZE = 4;
static constexpr uint8_t THUMB_INSTRUCTION_SIZE = 2;

// The following constants are masks used to identify the type of instruction.
// The order of the checks is important, so check in the order the constants are defined.
static constexpr uint32_t ARM_SOFTWARE_INTERRUPT_OPCODE = 0x0F000000;
static constexpr uint32_t ARM_COPROCESSOR_OPCODE = 3 << 26;
static constexpr uint32_t ARM_BRANCH_OPCODE = 5 << 25;
static constexpr uint32_t ARM_BLOCK_DATA_TRANSFER_OPCODE = 1 << 27;
static constexpr uint32_t ARM_UNDEFINED_OPCODE = (3 << 25) | (1 << 4);
static constexpr uint32_t ARM_SINGLE_DATA_TRANSFER_OPCODE = 1 << 26;
static constexpr uint32_t ARM_HALFWORD_DATA_TRANSFER_IMMEDIATE_OPCODE = (1 << 22) | (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_HALFWORD_DATA_TRANSFER_REGISTER_OPCODE = (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_HALFWORD_DATA_TRANSFER_SH_MASK = (1 << 5) | (1 << 6); // SH > 0 for halfword register data transfer
static constexpr uint32_t ARM_BRANCH_AND_EXCHANGE_OPCODE = 0x12FFF10;
static constexpr uint32_t ARM_SINGLE_DATA_SWAP_OPCODE = (1 << 24) | (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_MULTIPLY_LONG_OPCODE = (1 << 23) | (1 << 7) | (1 << 4);
static constexpr uint32_t ARM_MULTIPLY_OPCODE = (1 << 7) | (1 << 4); // SH == 0 for multiply
// Any other instruction is a Data Processing instruction.

static constexpr uint16_t THUMB_LONG_BRANCH_WITH_LINK_OPCODE = 0xF000;
static constexpr uint16_t THUMB_UNCONDITIONAL_BRANCH_OPCODE = 7 << 13;
static constexpr uint16_t THUMB_SOFTWARE_INTERRUPT_OPCODE = 0xDF00;
static constexpr uint16_t THUMB_CONDITIONAL_BRANCH_OPCODE = 0xD000;
static constexpr uint16_t THUMB_MULTIPLE_LOAD_STORE_OPCODE = 3 << 14;
static constexpr uint16_t THUMB_PUSH_POP_REGISTERS_OPCODE = 0x2D << 10;
static constexpr uint16_t THUMB_ADD_OFFSET_TO_STACK_POINTER_OPCODE = 0xB000;
static constexpr uint16_t THUMB_LOAD_ADDRESS_OPCODE = 0xA000;
static constexpr uint16_t THUMB_SP_RELATIVE_LOAD_STORE_OPCODE = 9 << 12;
static constexpr uint16_t THUMB_LOAD_STORE_HALFWORD_OPCODE = 1 << 15;
static constexpr uint16_t THUMB_LOAD_STORE_IMMEDIATE_OFFSET_OPCODE = 3 << 13;
static constexpr uint16_t THUMB_LOAD_STORE_SIGN_EXTENDED_BYTE_HALFWORD_OPCODE = 0x29 << 9;
static constexpr uint16_t THUMB_LOAD_STORE_REGISTER_OFFSET_OPCODE = 0x5 << 12;
static constexpr uint16_t THUMB_PC_RELATIVE_LOAD_OPCODE = 0x9 << 11;
static constexpr uint16_t THUMB_HI_REGISTER_OPERATIONS_BRANCH_EXCHANGE_OPCODE = 0x11 << 10;
static constexpr uint16_t THUMB_ALU_OPERATIONS_OPCODE = 1 << 14;
static constexpr uint16_t THUMB_MOV_CMP_ADD_SUB_IMMEDIATE_OPCODE = 1 << 13;
static constexpr uint16_t THUMB_ADD_SUB_OPCODE = 3 << 11;
// Any other instruction is a MOVE_SHIFTED_REGISTER instruction.

enum CPUOperatingMode {
  User = 0b10000,
  FIQ = 0b10001,
  IRQ = 0b10010,
  Supervisor = 0b10011,
  Abort = 0b10111,
  Undefined = 0b11011,
  System = 0b11111
};

static constexpr uint8_t FIQ_BANKED_REGISTERS_IDX = 0;
static constexpr uint8_t IRQ_BANKED_REGISTERS_IDX = 1;
static constexpr uint8_t SUPERVISOR_BANKED_REGISTERS_IDX = 2;
static constexpr uint8_t ABORT_BANKED_REGISTERS_IDX = 3;
static constexpr uint8_t UNDEFINED_BANKED_REGISTERS_IDX = 4;

enum ConditionCodes {
  EQ = 0b0000, // Z == 1, Equal
  NE = 0b0001, // Z == 0, Not Equal
  CS = 0b0010, // C == 1, Carry Set
  CC = 0b0011, // C == 0, Carry Clear
  MI = 0b0100, // N == 1, Minus
  PL = 0b0101, // N == 0, Plus
  VS = 0b0110, // V == 1, Overflow
  VC = 0b0111, // V == 0, No Overflow
  HI = 0b1000, // C == 1 and Z == 0, Higher
  LS = 0b1001, // C == 0 or Z == 1, Lower or Same
  GE = 0b1010, // N == V, Greater or Equal
  LT = 0b1011, // N != V, Less Than
  GT = 0b1100, // Z == 0 and N == V, Greater Than
  LE = 0b1101, // Z == 1 or N != V, Less or Equal
  AL = 0b1110, // Always (unconditional execution)
  NV = 0b1111  // Reserved, Ignore
};

// CSPR - Current Program Status Register
// Bit 31 - N (Negative / Less Than)
// Bit 30 - Z (Zero)
// Bit 29 - C (Carry / Borrow / Extend)
// Bit 28 - V (Overflow)
// Bit 27-8 - Reserved
// Bit 7 - IRQ Interrupt Disable
// Bit 6 - FIQ Interrupt Disable
// Bit 5 - State bit (Tbit, 0 = ARM, 1 = THUMB)
static constexpr uint32_t CSPR_N = 1 << 31;
static constexpr uint32_t CSPR_Z = 1 << 30;
static constexpr uint32_t CSPR_C = 1 << 29;
static constexpr uint32_t CSPR_V = 1 << 28;
static constexpr uint32_t CSPR_IRQ_DISABLE = 1 << 7;
static constexpr uint32_t CSPR_FIQ_DISABLE = 1 << 6;
static constexpr uint32_t CSPR_THUMB_STATE = 1 << 5;

enum SpecialRegisters {
  SP = 13, // Stack Pointer
  LR = 14, // Link Register
  PC = 15  // Program Counter
};

enum ShiftTypes {
  LOGICAL_LEFT = 0b00,
  LOGICAL_RIGHT = 0b01,
  ARITHMETIC_RIGHT = 0b10,
  ROTATE_RIGHT = 0b11
};

enum DataProcessingOpcodes {
  AND = 0b0000,
  EOR = 0b0001,
  SUB = 0b0010,
  RSB = 0b0011,
  ADD = 0b0100,
  ADC = 0b0101,
  SBC = 0b0110,
  RSC = 0b0111,
  TST = 0b1000,
  TEQ = 0b1001,
  CMP = 0b1010,
  CMN = 0b1011,
  ORR = 0b1100,
  MOV = 0b1101,
  BIC = 0b1110,
  MVN = 0b1111,
};

// ARM7TDMI CPU
// Used in the GameBoy Advance (SP)
// Manual: https://www.dwedit.org/files/ARM7TDMI.pdf
struct CPU {
  // Memory Mapper
  RAM ram;

  // ARM State - access to the 16 general-purpose registers (r0 - r15)
  //   Where r15 is the Program Counter (PC), r13 is the Stack Pointer (SP) and r14 is the Link Register (LR)
  //   Bits 0-1 of the PC are always 0, the rest are the address of the current instruction (4-byte aligned)
  //   So when reading the PC, we need to shift it right by 2 bits
  // THUMB State - access to the 8 general-purpose registers (r0 - r7)
  //   Where r15 is still the PC, r13 is the Stack Pointer (SP) and r14 is the Link Register (LR)
  //   Bit 0 of the PC is always 0, the rest are the address of the current instruction (2-byte aligned)
  //   So when reading the PC, we need to shift it right by 1 bit
  //   The other 8 registers are the high
  uint32_t registers[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, // r0 - r7
    0, 0, 0, 0, 0, 0, 0, 0  // r8 - r15
  };

  // For some CPU modes, the logical registers map to different physical registers (banked registers).
  uint32_t banked_registers[5][7] = {
    {0, 0, 0, 0, 0, 0, 0}, // FIQ
    {0, 0, 0, 0, 0, 0, 0}, // IRQ
    {0, 0, 0, 0, 0, 0, 0}, // Supervisor
    {0, 0, 0, 0, 0, 0, 0}, // Abort
    {0, 0, 0, 0, 0, 0, 0}  // Undefined
  };
  std::map<uint8_t, uint32_t> mode_to_banked_registers = {
    {FIQ, 0},
    {IRQ, 1},
    {Supervisor, 2},
    {Abort, 3},
    {Undefined, 4}
  };

  // CSPR - Current Program Status Register
  // Bit 31 - N (Negative / Less Than)
  // Bit 30 - Z (Zero)
  // Bit 29 - C (Carry / Borrow / Extend)
  // Bit 28 - V (Overflow)
  // Bit 27-8 - Reserved
  // Bit 7 - IRQ Interrupt Disable
  // Bit 6 - FIQ Interrupt Disable
  // Bit 5 - State bit (Tbit, 0 = ARM, 1 = THUMB)
  // Bit 4-0 - Mode bits (5 bits, see CPUOperatingMode)
  uint32_t cspr = (uint32_t)User;
  std::map<uint8_t, uint32_t> mode_to_scspr = {
    {FIQ, 0},
    {IRQ, 0},
    {Supervisor, 0},
    {Abort, 0},
    {Undefined, 0},
  };

  // ARM Data Processing Instruction Handlers
  std::map<uint32_t, std::vector<std::function<void(CPU&, uint8_t, uint8_t, uint16_t, uint8_t)>>> arm_data_processing_instructions;

  // Possible Conditions
  std::vector<uint32_t> conditions = {
    EQ, NE, CS, CC, MI, PL, VS, VC,
    HI, LS, GE, LT, GT, LE, AL, NV
  };

  uint32_t get_register_value(uint8_t reg) {
    uint8_t mode = cspr & 0x1F;
    switch (mode) {
      case FIQ:
        if (reg >= 8 && reg <= 14) {
          return banked_registers[FIQ_BANKED_REGISTERS_IDX][reg - 8];
        }
      case IRQ:
      case Supervisor:
      case Abort:
      case Undefined:
        if (reg == 13 || reg == 14) {
          return banked_registers[mode_to_banked_registers[mode]][reg - 13];
        }
      default:
        return registers[reg];
    }
  }

  void set_register_value(uint8_t reg, uint32_t value) {
    uint8_t mode = cspr & 0x1F;
    switch (mode) {
      case FIQ:
        if (reg >= 8 && reg <= 14) {
          banked_registers[FIQ_BANKED_REGISTERS_IDX][reg - 8] = value;
          return;
        }
      case IRQ:
      case Supervisor:
      case Abort:
      case Undefined:
        if (reg == 13 || reg == 14) {
          banked_registers[mode_to_banked_registers[mode]][reg - 13] = value;
          return;
        }
      default:
        registers[reg] = value;
    }
  }

  uint8_t get_instruction_size() {
    return cspr & CSPR_THUMB_STATE ? THUMB_INSTRUCTION_SIZE : ARM_INSTRUCTION_SIZE;
  }

  void increment_pc() {
    set_register_value(PC, get_register_value(PC) + get_instruction_size());
  }
};

void cpu_init(CPU& cpu);
void cpu_cycle(CPU& cpu);
