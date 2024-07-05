#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Multiply", "[arm, multiply]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  SECTION("MUL") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0x0;

    // MUL R0, R1, R2
    cpu.set_register_value(1, 0x2);
    cpu.set_register_value(2, 0x3);

    cpu_cycle(cpu);
    REQUIRE(cpu.get_register_value(0) == 0x6);
  }

  SECTION("MLA") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0x4;

    // MLA R0, R1, R2, R3
    cpu.set_register_value(1, 0x2);
    cpu.set_register_value(2, 0x3);
    cpu.set_register_value(3, 0x4);

    cpu_cycle(cpu);
    REQUIRE(cpu.get_register_value(0) == 10);
  }

  SECTION("UMULL") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0x8;

    // UMULL R0, R1, R2, R3
    cpu.set_register_value(2, 0x2);
    cpu.set_register_value(3, 0xFFFFFFFF);

    cpu_cycle(cpu);
    REQUIRE(cpu.get_register_value(0) == 0xFFFFFFFE);
    REQUIRE(cpu.get_register_value(1) == 0x1);
  }

  SECTION("UMLAL") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0xC;

    // UMLAL R0, R1, R2, R3
    cpu.set_register_value(2, 0x2);
    cpu.set_register_value(3, 0xFFFFFFFF);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x0);

    // = 0x1 + 0xFFFFFFFF * 0x2
    cpu_cycle(cpu);
    REQUIRE(cpu.get_register_value(0) == 0xFFFFFFFF);
    REQUIRE(cpu.get_register_value(1) == 0x1);
  }

  SECTION("SMULL") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0x10;

    // SMULL R0, R1, R2, R3
    cpu.set_register_value(2, (uint32_t)-2);
    cpu.set_register_value(3, 0x7FFFFFFF);

    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x2);
    REQUIRE(cpu.get_register_value(1) == 0xFFFFFFFF);
  }

  SECTION("SMLAL") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_multiply.bin"));
    cpu.registers[PC] = 0x14;

    // SMLAL R0, R1, R2, R3
    cpu.set_register_value(2, (uint32_t)-2);
    cpu.set_register_value(3, 0x7FFFFFFF);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x0);

    // = 0x1 + 0x7FFFFFFF * -2
    cpu_cycle(cpu);
    REQUIRE(cpu.get_register_value(0) == 0x3);
    REQUIRE(cpu.get_register_value(1) == 0xFFFFFFFF);
  }
}
