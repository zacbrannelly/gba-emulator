#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Unconditional Branch", "[thumb, unconditional-branch]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/unconditional_branch.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("B") {
    cpu.set_register_value(PC, 0x0);

    // b to instruction at 0x2
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x2);
  }
}
