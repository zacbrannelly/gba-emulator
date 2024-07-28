#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Software Interrupt", "[thumb, software-interrupt]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/software_interrupt.bin"));

  // Enter Thumb State
  cpu.cpsr = (uint32_t)System | CPSR_THUMB_STATE;

  SECTION("SWI") {
    cpu.set_register_value(PC, 0x0);

    // swi 18
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x8);
    REQUIRE(cpu.get_register_value(LR) == 0x2);
    REQUIRE(cpu.cpsr == (uint32_t)Supervisor);
    REQUIRE(cpu.mode_to_scpsr[Supervisor] == ((uint32_t)System | CPSR_THUMB_STATE));
  }
}
