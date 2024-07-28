#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("PC-Relative Load", "[thumb, pc-relative-load]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("LDR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/pc_relative_load.bin"));

    cpu.set_register_value(PC, 0);
    ram_write_word(cpu.ram, 2 * cpu.get_instruction_size() + 4, 0x12345678);

    // ldr r0, [pc, #4]
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("LDR using PC and data in the binary") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/pc_relative_load_from_program.bin"));

    cpu.set_register_value(PC, 0);

    // ldr r0, _data, where _data is 0x04000110
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x04000110);

    // mov r0, #0
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0);

    // ldr r0, _data, where _data is 0x04000110
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x04000110);
  }
}
