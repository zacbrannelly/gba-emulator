#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Multiple Load/Store", "[thumb, multiple-load-store]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/multiple_load_store.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("STMIA") {
    cpu.set_register_value(PC, 0x0);

    // stmia r0!, {r1, r2}
    cpu.set_register_value(0, 0x2000);
    cpu.set_register_value(1, 0x12345678);
    cpu.set_register_value(2, 0x87654321);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x2000) == 0x12345678);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 + 4) == 0x87654321);
    REQUIRE(cpu.get_register_value(0) == 0x2000 + 8);
  }

  SECTION("LDMIA") {
    cpu.set_register_value(PC, 0x2);

    // ldmia r0!, {r1, r2}
    ram_write_word(cpu.ram, 0x2000, 0x12345678);
    ram_write_word(cpu.ram, 0x2000 + 4, 0x87654321);
    cpu.set_register_value(0, 0x2000);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12345678);
    REQUIRE(cpu.get_register_value(2) == 0x87654321);
    REQUIRE(cpu.get_register_value(0) == 0x2000 + 8);
  }
}
