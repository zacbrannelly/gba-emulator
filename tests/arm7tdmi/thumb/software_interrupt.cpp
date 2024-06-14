#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Software Interrupt", "[thumb, software-interrupt]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/software_interrupt.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("SWI") {
    cpu.set_register_value(PC, 0x0);

    // swi 18
    cpu_cycle(cpu);

    REQUIRE(cpu.registers[PC] == 0x8);
    REQUIRE(cpu.cspr == (uint32_t)Supervisor);
    REQUIRE(cpu.mode_to_scspr[Supervisor] == ((uint32_t)User | CSPR_THUMB_STATE));
  }
}
