#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Load address relative to PC/SP", "[thumb, load-address]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/load_address.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("Load address relative to PC") {
    cpu.set_register_value(PC, 0x0);

    // add r0, pc, #4
    cpu_cycle(cpu);

    uint32_t expected = 0x4 + 2 * cpu.get_instruction_size();
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("Load address relative to SP") {
    cpu.set_register_value(PC, 0x2);

    // add r0, sp, #4
    cpu.set_register_value(SP, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x2004;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
