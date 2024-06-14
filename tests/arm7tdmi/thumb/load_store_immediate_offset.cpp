#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Load/Store w/ Immediate Offset", "[thumb, load-store-immediate-offset]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/load_store_immediate_offset.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("STR") {
    cpu.set_register_value(PC, 0x0);

    // str r0, [r1, #4]
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(ram_read_word(cpu.ram, 0x2004) == expected);
  }

  SECTION("LDR") {
    cpu.set_register_value(PC, 0x2);

    // ldr r0, [r1, #4]
    ram_write_word(cpu.ram, 0x2004, 0x12345678);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("STRB") {
    cpu.set_register_value(PC, 0x4);

    // strb r0, [r1, #4]
    cpu.set_register_value(0, 0x78);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x78;
    REQUIRE(ram_read_byte(cpu.ram, 0x2004) == expected);
  }

  SECTION("LDRB") {
    cpu.set_register_value(PC, 0x6);

    // ldrb r0, [r1, #4]
    ram_write_byte(cpu.ram, 0x2004, 0x78);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x78;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
