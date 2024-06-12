#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Load/Store Unsigned/Signed Byte/Halfword", "[thumb, load-store-signed-halfword]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/load_store_signed_halfword.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("STRH") {
    cpu.set_register_value(PC, 0x0);

    // strh r0, [r1, r2]
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x2000);
    cpu.set_register_value(2, 4);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(*(uint32_t*)&cpu.memory[0x2004] == expected);
  }

  SECTION("LDRH") {
    cpu.set_register_value(PC, 0x2);

    // ldrh r0, [r1, r2]
    *(uint16_t*)&cpu.memory[0x2004] = 0x5678;
    cpu.set_register_value(1, 0x2000);
    cpu.set_register_value(2, 4);
    cpu_cycle(cpu);

    uint32_t expected = 0x5678;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("LDSB") {
    cpu.set_register_value(PC, 0x4);

    // ldsb r0, [r1, r2]
    cpu.memory[0x2004] = (uint8_t)-4;
    cpu.set_register_value(1, 0x2000);
    cpu.set_register_value(2, 4);
    cpu_cycle(cpu);

    uint32_t expected = (uint32_t)-4;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("LDSH") {
    cpu.set_register_value(PC, 0x6);

    // ldsh r0, [r1, r2]
    *(uint16_t*)&cpu.memory[0x2004] = (uint16_t)-4;
    cpu.set_register_value(1, 0x2000);
    cpu.set_register_value(2, 4);
    cpu_cycle(cpu);

    uint32_t expected = (uint32_t)-4;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}