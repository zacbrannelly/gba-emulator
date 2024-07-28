#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("SP-Relative Load/Store", "[thumb, sp-relative-load-store]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/sp_relative_load_store.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("STR") {
    cpu.set_register_value(PC, 0x0);

    // str r0, [sp, #4]
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(SP, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(ram_read_word(cpu.ram, 0x2004) == expected);
  }

  SECTION("LDR") {
    cpu.set_register_value(PC, 0x2);

    // ldr r0, [sp, #4]
    ram_write_word(cpu.ram, 0x2004, 0x12345678);
    cpu.set_register_value(SP, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
