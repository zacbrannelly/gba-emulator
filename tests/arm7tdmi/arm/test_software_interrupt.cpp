#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Software Interrupt", "[arm, software-interrupt]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  SECTION("SWI") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_software_interrupt.bin"));
    cpu.registers[PC] = 0x0;
    cpu.cspr = (uint32_t)User;

    // swi 1
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x8);
    REQUIRE(cpu.get_register_value(LR) == 0x4);
    REQUIRE(cpu.cspr == (uint32_t)Supervisor);
    REQUIRE(cpu.mode_to_scspr[Supervisor] == (uint32_t)User);
  }
}
