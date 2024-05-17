#include <cstdint>
#include <vector>
#include <map>
#include <functional>

constexpr uint8_t ARM_INSTRUCTION_SIZE = 4;
constexpr uint8_t THUMB_INSTRUCTION_SIZE = 2;

enum CPUOperatingMode {
  User = 0b10000,
  FIQ = 0b10001,
  IRQ = 0b10010,
  Supervisor = 0b10011,
  Abort = 0b10111,
  Undefined = 0b11011,
  System = 0b11111
};

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
  // 65kb of memory
  uint8_t* memory = new uint8_t[0x10000];

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
  // TODO: Figure out the address of the CPSR
  // TODO: Figure out the address of the SPSR's
  uint32_t cspr = 0;
  std::map<uint8_t, uint32_t> mode_to_scspr = {
    {FIQ, 0},
    {IRQ, 0},
    {Supervisor, 0},
    {Abort, 0},
    {Undefined, 0},
  };

  // ARM Instruction Set (each condition code is set to zero)
  std::map<uint32_t, std::function<void(CPU&)>> arm_instructions;


  // Possible Conditions
  std::vector<uint32_t> conditions = {
    EQ, NE, CS, CC, MI, PL, VS, VC,
    HI, LS, GE, LT, GT, LE, AL, NV
  };
};

void cpu_init(CPU& cpu);
void cpu_cycle(CPU& cpu);
