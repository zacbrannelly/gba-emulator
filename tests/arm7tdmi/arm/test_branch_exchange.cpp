#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Branch and Exchange (BX)", "[arm, branch-exchange]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  SECTION("ARM Mode") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_exchange.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;
    cpu.registers[0] = 0x0; // R0 is the branch address, with first bit set to 0 to indicate ARM mode.
    uint32_t cpsr = cpu.cpsr = (uint32_t)User;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x0);
    REQUIRE(cpu.cpsr == cpsr); // Should not change the mode.
  }

  SECTION("THUMB Mode") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_exchange.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;
    cpu.registers[0] = 0x5; // R0 is the branch address, with first bit set to 1 to indicate THUMB mode.
    cpu.cpsr = (uint32_t)User;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x4);

    uint32_t expected_cpsr = (uint32_t)User | (1 << 5); // Set the Tbit to 1.
    REQUIRE(cpu.cpsr == expected_cpsr);
  }
}
