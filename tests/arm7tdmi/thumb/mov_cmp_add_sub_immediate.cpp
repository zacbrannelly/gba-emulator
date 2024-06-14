#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Move, compare, add, subtract (immediate)", "[thumb, mov-cmp-add-sub-immediate]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/mov_cmp_add_sub_immediate.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("MOV") {
    cpu.set_register_value(PC, 0);

    // mov r0, #5
    cpu.set_register_value(0, 0);
    cpu_cycle(cpu);

    int expected = 5;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("CMP") {
    cpu.set_register_value(PC, 0x2);

    // cmp r0, #5
    cpu.set_register_value(0, 5);
    cpu_cycle(cpu);

    bool result = cpu.cspr & CSPR_Z;
    REQUIRE(result == true);
  }

  SECTION("ADD") {
    cpu.set_register_value(PC, 0x4);

    // add r0, #5
    cpu.set_register_value(0, 0);
    cpu_cycle(cpu);

    int expected = 5;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("SUB") {
    cpu.set_register_value(PC, 0x6);

    // sub r0, #5
    cpu.set_register_value(0, 10);
    cpu_cycle(cpu);

    int expected = 5;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
