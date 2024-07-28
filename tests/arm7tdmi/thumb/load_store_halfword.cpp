#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Load/Store Halfword w/ Immediate Offset", "[thumb, load-store-halfword-immediate-offset]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/load_store_halfword.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("STR") {
    cpu.set_register_value(PC, 0x0);

    // strh r0, [r1, #4]
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(ram_read_word(cpu.ram, 0x2004) == expected);
  }

  SECTION("LDR") {
    cpu.set_register_value(PC, 0x2);

    // ldrh r0, [r1, #4]
    ram_write_half_word(cpu.ram, 0x2004, 0x5678);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
