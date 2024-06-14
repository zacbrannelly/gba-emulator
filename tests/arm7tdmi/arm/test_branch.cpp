#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Branch (B)", "[arm, branch]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;


  SECTION("Infinite Loop Program") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_infinite_loop.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;

    for (int i = 0; i < 5; i++) {
      cpu_cycle(cpu);
      REQUIRE(cpu.registers[PC] == 0x4);
      REQUIRE(cpu.registers[LR] == 0x0); // Should not save the return address.
    }
  }

  SECTION("Jump Ahead Program") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_ahead.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;

    for (int i = 0; i < 5; i++) {
      cpu_cycle(cpu);
      REQUIRE(cpu.registers[PC] == 0x10); // Should jump 16 bytes ahead and loop infinitely.
      REQUIRE(cpu.registers[LR] == 0x0); // Should not save the return address.
    }
  }

  SECTION("Jump Before Program") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_before.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x14); // Should jump 20 bytes ahead first.
    REQUIRE(cpu.registers[LR] == 0x0); // Should not save the return address.

    for (int i = 0; i < 5; i++) {
      cpu_cycle(cpu);
      REQUIRE(cpu.registers[PC] == 0x10); // Should jump back 4 bytes and loop infinitely.
    }
  }

  SECTION("Branch With Link Program") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_branch_link.bin"));
    cpu.registers[PC] = 0x0;
    cpu.registers[LR] = 0x0;

    cpu_cycle(cpu);
    REQUIRE(cpu.registers[PC] == 0x8); // Should jump 8 bytes.
    REQUIRE(cpu.registers[LR] == 0x4); // Should save the return address.
  }
}
