#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Long Branch w/ Link", "[thumb, long-branch-with-link]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/long_branch_with_link.bin"));

  // Enter Thumb State
  cpu.cpsr |= CPSR_THUMB_STATE;

  SECTION("BL") {
    cpu.set_register_value(PC, 0x0);

    // bl to instruction at 0x4
    // return instruction is saved in LR (0x2)
    cpu_cycle(cpu);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x6);

    // LR should be 0x4 with bit 0 set, so 0x5
    REQUIRE(cpu.get_register_value(LR) == 0x5);
  }
}
