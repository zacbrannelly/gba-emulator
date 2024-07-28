#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Push/Pop Registers", "[thumb, push-pop-registers]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/push_pop_registers.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("PUSH") {
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(SP, 0x2000);
    
    // push {r0, r1, r2}
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x87654321);
    cpu.set_register_value(2, 0x11111111);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 4) == 0x11111111);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 8) == 0x87654321);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 12) == 0x12345678);
    REQUIRE(cpu.get_register_value(SP) == 0x2000 - 12);

    cpu.set_register_value(PC, 0x2);
    cpu.set_register_value(SP, 0x2000);

    // push {r0, r1, r2, lr}
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x87654321);
    cpu.set_register_value(2, 0x11111111);
    cpu.set_register_value(LR, 0x2);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 4) == 0x2);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 8) == 0x11111111);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 12) == 0x87654321);
    REQUIRE(ram_read_word(cpu.ram, 0x2000 - 16) == 0x12345678);
    REQUIRE(cpu.get_register_value(SP) == 0x2000 - 16);
  }

  SECTION("POP") {
    cpu.set_register_value(PC, 0x4);
    cpu.set_register_value(SP, 0x2000 - 12);

    // pop {r0, r1, r2} ; post-increment load
    ram_write_word(cpu.ram, 0x2000 - 12, 0x12345678); // r0
    ram_write_word(cpu.ram, 0x2000 - 8, 0x87654321); // r1
    ram_write_word(cpu.ram, 0x2000 - 4, 0x11111111); // r2
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12345678);
    REQUIRE(cpu.get_register_value(1) == 0x87654321);
    REQUIRE(cpu.get_register_value(2) == 0x11111111);
    REQUIRE(cpu.get_register_value(SP) == 0x2000);

    // pop {r0, r1, r2, pc}
    cpu.set_register_value(SP, 0x2000 - 12);
    ram_write_word(cpu.ram, 0x2000, 0x2);
    ram_write_word(cpu.ram, 0x2000 - 4, 0x12345678);
    ram_write_word(cpu.ram, 0x2000 - 8, 0x87654321);
    ram_write_word(cpu.ram, 0x2000 - 12, 0x11111111);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x11111111);
    REQUIRE(cpu.get_register_value(1) == 0x87654321);
    REQUIRE(cpu.get_register_value(2) == 0x12345678);
    REQUIRE(cpu.get_register_value(PC) == 0x2);
    REQUIRE(cpu.get_register_value(SP) == 0x2004);
  }
}
