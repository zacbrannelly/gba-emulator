#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Single Data Transfer", "[arm, single-data-transfer]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;


  SECTION("LDR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_single_data_transfer.bin"));
    cpu.registers[PC] = 0x0;

    // ldr r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_word(cpu.ram, 0x64, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);

    // ldr r0, [r1, #4]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_word(cpu.ram, 0x68, 0x34343434);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x34343434);

    // ldr r0, [r1, r2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    ram_write_word(cpu.ram, 0x68, 0x56565656);
    cpu_cycle(cpu);
    
    REQUIRE(cpu.get_register_value(0) == 0x56565656);

    // ldr r0, [r1, r2, lsl #2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    ram_write_word(cpu.ram, 0x68, 0x78787878);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x78787878);

    // ldr r0, [r1, r2, lsl #2]!
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    ram_write_word(cpu.ram, 0x68, 0x78787878);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x78787878);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldr r0, [r1], r2
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    ram_write_word(cpu.ram, 0x64, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldr r0, [r1], r2, lsl #2
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    ram_write_word(cpu.ram, 0x64, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldrb r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    ram_write_byte(cpu.ram, 0x64, 0x12);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12);

    // ldr r0, [r1, -r2]
    ram_write_word(cpu.ram, 0x60, 0x12121212);
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
  }

  SECTION("STR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_single_data_transfer.bin"));
    cpu.registers[PC] = 0x24;

    // str r0, [r1]
    ram_write_word(cpu.ram, 0x64, 0x0);
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x64) == 0x12121212);

    // str r0, [r1, #4]
    ram_write_word(cpu.ram, 0x68, 0x0);
    cpu.set_register_value(0, 0x34343434);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x34343434);

    // str r0, [r1, r2]
    ram_write_word(cpu.ram, 0x68, 0x0);
    cpu.set_register_value(0, 0x56565656);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x56565656);

    // str r0, [r1, r2, lsl #2]
    ram_write_word(cpu.ram, 0x68, 0x0);
    cpu.set_register_value(0, 0x78787878);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x78787878);

    // str r0, [r1, r2, lsl #2]!
    ram_write_word(cpu.ram, 0x68, 0x0);
    cpu.set_register_value(0, 0x78787878);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x78787878);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // str r0, [r1], r2
    ram_write_word(cpu.ram, 0x64, 0x0);
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x64) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // str r0, [r1], r2, lsl #2
    ram_write_word(cpu.ram, 0x64, 0x0);
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x64) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // strb r0, [r1]
    ram_write_word(cpu.ram, 0x64, 0x0);
    cpu.set_register_value(0, 0x12);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(ram_read_byte(cpu.ram, 0x64) == 0x12);

    // str r0, [r1, -r2]
    ram_write_word(cpu.ram, 0x60, 0x0);
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x60) == 0x12121212);
  }
}
