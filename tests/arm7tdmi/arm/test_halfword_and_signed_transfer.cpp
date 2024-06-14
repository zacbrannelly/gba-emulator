#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Halfword and Signed Transfer", "[arm, halfword-and-signed-transfer]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;


  SECTION("LDRH") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_halfword_and_signed_transfer.bin"));
    cpu.registers[PC] = 0x0;

    // ldrh r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_half_word(cpu.ram, 0x64, 0x1212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x1212);

    // ldrh r0, [r1, #4]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_half_word(cpu.ram, 0x68, 0x3434);

    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x3434);

    // ldrh r0, [r1, r2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    ram_write_half_word(cpu.ram, 0x68, 0x5656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x5656);

    // ldrh r0, [r1, r2]!
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    ram_write_half_word(cpu.ram, 0x68, 0x5656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x5656);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldrh r0, [r1], r2
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    ram_write_half_word(cpu.ram, 0x64, 0x7878);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x7878);
    REQUIRE(cpu.get_register_value(1) == 0x68);
  }

  SECTION("LDRSH/LDRSB") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_halfword_and_signed_transfer.bin"));
    cpu.registers[PC] = 0x14;

    // ldrsh r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_half_word(cpu.ram, 0x64, (uint16_t)-300);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == -300);

    // ldrsb r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_byte(cpu.ram, 0x64, (uint8_t)-100);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == -100);
  }
}