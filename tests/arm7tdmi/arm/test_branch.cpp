#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Branch (B)", "[arm, branch]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  SECTION("Infinite Loop Program") {
    // Load infinite loop ROM
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_branch_infinite_loop.bin"));
    cpu.registers[PC] = 0x0;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x4);

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x4);

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x4);
  }

  SECTION("Jump Ahead Program") {
    // Load jump ahead ROM
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_branch_ahead.bin"));
    cpu.registers[PC] = 0x0;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10); // Should jump 16 bytes ahead and loop infinitely.

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10);

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10);
  }

  SECTION("Jump Before Program") {
    // Load jump before ROM
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_branch_before.bin"));
    cpu.registers[PC] = 0x0;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x14); // Should jump 20 bytes ahead first.

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10); // Should jump back 4 bytes and loop infinitely.

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10);

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x10);
  }
}
