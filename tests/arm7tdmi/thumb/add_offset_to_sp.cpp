#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Add Offset To SP", "[thumb, add-offset-to-sp]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/add_offset_to_sp.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("ADD") {
    cpu.set_register_value(PC, 0x0);

    // add sp, #4
    cpu.set_register_value(SP, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x2004;
    REQUIRE(cpu.get_register_value(SP) == expected);
  }

  SECTION("SUB") {
    cpu.set_register_value(PC, 0x2);

    // add sp, #-4
    cpu.set_register_value(SP, 0x2004);
    cpu_cycle(cpu);

    uint32_t expected = 0x2000;
    REQUIRE(cpu.get_register_value(SP) == expected);
  }
}
