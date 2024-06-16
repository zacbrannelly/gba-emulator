#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Push / Pop from Stack", "[arm, push-pop]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Set SP to 0x3007F00 (SP for System mode)
  // This is required for the PUSH and POP instructions to work.
  cpu.set_register_value(SP, 0x3007F00);

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_push_pop.bin"));

  SECTION("PUSH/POP") {
    cpu.set_register_value(PC, 0);

    // push {r4, r5, r6, r7, r8, r9, r10, lr}
    cpu.set_register_value(4, 0x12345678);
    cpu.set_register_value(5, 0x87654321);
    cpu.set_register_value(6, 0x11111111);
    cpu.set_register_value(7, 0x22222222);
    cpu.set_register_value(8, 0x33333333);
    cpu.set_register_value(9, 0x44444444);
    cpu.set_register_value(10, 0x55555555);
    cpu.set_register_value(LR, 0x2);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 4) == 0x2);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 8) == 0x55555555);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 12) == 0x44444444);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 16) == 0x33333333);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 20) == 0x22222222);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 24) == 0x11111111);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 28) == 0x87654321);
    REQUIRE(ram_read_word(cpu.ram, 0x3007F00 - 32) == 0x12345678);
    REQUIRE(cpu.get_register_value(SP) == 0x3007F00 - 32);

    // pop {r4, r5, r6, r7, r8, r9, r10, lr}
    cpu.set_register_value(4, 0);
    cpu.set_register_value(5, 0);
    cpu.set_register_value(6, 0);
    cpu.set_register_value(7, 0);
    cpu.set_register_value(8, 0);
    cpu.set_register_value(9, 0);
    cpu.set_register_value(10, 0);
    cpu.set_register_value(LR, 0);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(4) == 0x12345678);
    REQUIRE(cpu.get_register_value(5) == 0x87654321);
    REQUIRE(cpu.get_register_value(6) == 0x11111111);
    REQUIRE(cpu.get_register_value(7) == 0x22222222);
    REQUIRE(cpu.get_register_value(8) == 0x33333333);
    REQUIRE(cpu.get_register_value(9) == 0x44444444);
    REQUIRE(cpu.get_register_value(10) == 0x55555555);
    REQUIRE(cpu.get_register_value(LR) == 0x2);
  }
}
