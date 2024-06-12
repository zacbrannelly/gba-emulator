#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("PC-Relative Load", "[thumb, pc-relative-load]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/pc_relative_load.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("LDR") {
    cpu.set_register_value(PC, 0);
    *(uint32_t*)&cpu.memory[2 * cpu.get_instruction_size() + 4] = 0x12345678;

    // ldr r0, [pc, #4]
    cpu_cycle(cpu);

    uint32_t expected = 0x12345678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
