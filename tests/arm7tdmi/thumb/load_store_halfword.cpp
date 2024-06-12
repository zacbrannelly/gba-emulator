#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Load/Store Halfword w/ Immediate Offset", "[thumb, load-store-halfword-immediate-offset]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/load_store_halfword.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("STR") {
    cpu.set_register_value(PC, 0x0);

    // strh r0, [r1, #4]
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(*(uint32_t*)&cpu.memory[0x2004] == expected);
  }

  SECTION("LDR") {
    cpu.set_register_value(PC, 0x2);

    // ldrh r0, [r1, #4]
    *(uint16_t*)&cpu.memory[0x2004] = 0x5678;
    cpu.set_register_value(1, 0x2000);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
