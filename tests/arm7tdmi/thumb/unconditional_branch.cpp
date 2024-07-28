#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Unconditional Branch", "[thumb, unconditional-branch]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/unconditional_branch.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("B") {
    cpu.set_register_value(PC, 0x0);

    // b to instruction at 0x2
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x2);
  }
}
