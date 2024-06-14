#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("PC-Relative Load", "[thumb, pc-relative-load]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/pc_relative_load.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("LDR") {
    cpu.set_register_value(PC, 0);
    ram_write_word(cpu.ram, 2 * cpu.get_instruction_size() + 4, 0x12345678);

    // ldr r0, [pc, #4]
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
