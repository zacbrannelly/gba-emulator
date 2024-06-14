#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Load address relative to PC/SP", "[thumb, load-address]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/load_address.bin"));

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
