#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Branch and Exchange (BX)", "[arm, branch-exchange]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  SECTION("ARM Mode") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_branch_exchange.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;
    cpu.registers[0] = 0x0; // R0 is the branch address, with first bit set to 0 to indicate ARM mode.
    uint32_t cspr = cpu.cspr = (uint32_t)User;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x0);
    REQUIRE(cpu.cspr == cspr); // Should not change the mode.
  }

  SECTION("THUMB Mode") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_branch_exchange.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;
    cpu.registers[0] = 0x5; // R0 is the branch address, with first bit set to 1 to indicate THUMB mode.
    cpu.cspr = (uint32_t)User;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x4);

    uint32_t expected_cspr = (uint32_t)User | (1 << 5); // Set the Tbit to 1.
    REQUIRE(cpu.cspr == expected_cspr);
  }
}
