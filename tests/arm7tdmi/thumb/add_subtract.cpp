#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Add / Subtract (Register)", "[thumb, add-subtract-register]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/add_subtract.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("ADD") {
    cpu.set_register_value(PC, 0x0);

    // add r0, r1, r2
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 1);
    cpu.set_register_value(2, 2);
    cpu_cycle(cpu);

    int expected = 1 + 2;
    REQUIRE(cpu.get_register_value(0) == expected);

    // add r0, r1, #5
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 1);
    cpu_cycle(cpu);

    expected = 1 + 5;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("SUB") {
    cpu.set_register_value(PC, 0x4);

    // sub r0, r1, r2
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 5);
    cpu.set_register_value(2, 2);
    cpu_cycle(cpu);

    int expected = 5 - 2;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
