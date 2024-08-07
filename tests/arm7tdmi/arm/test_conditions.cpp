#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

void test_instruction(CPU& cpu, uint32_t expected) {
  cpu.registers[0] = 0x12121212;
  cpu_cycle(cpu);
  REQUIRE(cpu.registers[0] == expected);
}

TEST_CASE("Conditional Execution", "[arm, conditionals]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_conditions.bin"));

  SECTION("Zero Flag (Z)") {
    cpu.registers[PC] = 0;

    // EQ - Z == 1
    cpu.cpsr = CPSR_Z | (uint32_t)User;
    test_instruction(cpu, 1);

    // NE - Z == 0
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 0);

  }

  SECTION("Carry Flag (C)") {
    cpu.registers[PC] = 8;

    // CS - C == 1
    cpu.cpsr = CPSR_C | (uint32_t)User;
    test_instruction(cpu, 1);

    // CC - C == 0
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Negative Flag (N)") {
    cpu.registers[PC] = 16;

    // MI - N == 1
    cpu.cpsr = CPSR_N | (uint32_t)User;
    test_instruction(cpu, 1);

    // PL - N == 0
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Overflow Flag (V)") {
    cpu.registers[PC] = 24;

    // VS - V == 1
    cpu.cpsr = CPSR_V | (uint32_t)User;
    test_instruction(cpu, 1);

    // VC - V == 0
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Carry Flag (C) and Zero Flag (Z)") {
    cpu.registers[PC] = 32;

    // HI - C == 1 && Z == 0
    cpu.cpsr = CPSR_C | (uint32_t)User;
    test_instruction(cpu, 1);

    // LS - C == 0 || Z == 1
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 0);

    // Rolling back to the previous instruction
    cpu.set_register_value(PC, cpu.get_register_value(PC) - 4);

    // LS - C == 0 || Z == 1
    cpu.cpsr = CPSR_Z | (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Greater or Equal (GE) / Less Than (LT)") {
    cpu.registers[PC] = 40;

    // GE - N == V
    cpu.cpsr = CPSR_N | CPSR_V | (uint32_t)User;
    test_instruction(cpu, 1);

    // Rolling back to the previous instruction
    cpu.set_register_value(PC, cpu.get_register_value(PC) - 4);

    // GE - N == V
    cpu.cpsr = (uint32_t)User;
    test_instruction(cpu, 1);

    // LT - N != V
    cpu.cpsr = CPSR_N | (uint32_t)User;
    test_instruction(cpu, 0);

    // Rolling back to the previous instruction
    cpu.set_register_value(PC, cpu.get_register_value(PC) - 4);

    // LT - N != V
    cpu.cpsr = CPSR_V | (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Greater Than (GT) / Less or Equal (LE)") {
    cpu.registers[PC] = 48;

    // GT - Z == 0 && N == V
    cpu.cpsr = CPSR_N | CPSR_V | (uint32_t)User;
    test_instruction(cpu, 1);

    // LE - Z == 1 || N != V
    cpu.cpsr = CPSR_Z | CPSR_N | (uint32_t)User;
    test_instruction(cpu, 0);
  }

  SECTION("Always (AL)") {
    cpu.registers[PC] = 56;

    // AL - Always
    test_instruction(cpu, 1);
  }
}
