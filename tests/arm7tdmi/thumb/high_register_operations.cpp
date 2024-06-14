#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("High Register Operations", "[thumb, high-register-operations]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/high_register_operations.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("ADD") {
    cpu.set_register_value(PC, 0x0);

    // add r0, r9
    cpu.set_register_value(0, 0);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    int expected = 5;
    REQUIRE(cpu.get_register_value(0) == expected);

    // add r9, r0
    cpu.set_register_value(0, 5);
    cpu.set_register_value(9, 0);
    cpu_cycle(cpu);

    expected = 5;
    REQUIRE(cpu.get_register_value(9) == expected);

    // add r8, r9
    cpu.set_register_value(8, 0);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    expected = 5;
    REQUIRE(cpu.get_register_value(8) == expected);
  }

  SECTION("CMP") {
    cpu.set_register_value(PC, 0x6);

    // cmp r0, r9
    cpu.set_register_value(0, 5);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    bool result = cpu.cspr & CSPR_Z;
    REQUIRE(result);
    
    // cmp r9, r0
    cpu.set_register_value(0, 5);
    cpu.set_register_value(9, 6);
    cpu_cycle(cpu);

    result = cpu.cspr & CSPR_Z;
    REQUIRE_FALSE(result);

    // cmp r8, r9
    cpu.set_register_value(8, 5);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    result = cpu.cspr & CSPR_Z;
    REQUIRE(result);
  }

  SECTION("MOV") {
    cpu.set_register_value(PC, 0xC);

    // mov r0, r9
    cpu.set_register_value(0, 0);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    int expected = 5;
    REQUIRE(cpu.get_register_value(0) == expected);

    // mov r9, r0
    cpu.set_register_value(0, 5);
    cpu.set_register_value(9, 0);
    cpu_cycle(cpu);

    expected = 5;
    REQUIRE(cpu.get_register_value(9) == expected);

    // mov r8, r9
    cpu.set_register_value(8, 0);
    cpu.set_register_value(9, 5);
    cpu_cycle(cpu);

    expected = 5;
    REQUIRE(cpu.get_register_value(8) == expected);
  }

  SECTION("BX") {
    cpu.set_register_value(PC, 0x12);

    // bx r0
    cpu.set_register_value(0, 0x0);
    cpu_cycle(cpu);

    uint32_t expected = 0x0;
    REQUIRE(cpu.get_register_value(PC) == expected);
    REQUIRE_FALSE(cpu.cspr & CSPR_THUMB_STATE);

    // bx r9
    cpu.cspr |= CSPR_THUMB_STATE;
    cpu.set_register_value(PC, 0x14);
    cpu.set_register_value(9, 0x1);
    cpu_cycle(cpu);

    expected = 0x0;
    REQUIRE(cpu.get_register_value(PC) == expected);
    REQUIRE(cpu.cspr & CSPR_THUMB_STATE);
  }
}
