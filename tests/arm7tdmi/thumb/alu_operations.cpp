#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("ALU Operations", "[thumb, alu-operations]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/alu_operations.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("AND") {
    cpu.set_register_value(PC, 0x0);

    // and rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = 0b1001;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("EOR") {
    cpu.set_register_value(PC, 0x2);

    // eor rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = 0b0110;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("LSL") {
    cpu.set_register_value(PC, 0x4);

    // lsl rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 1);
    cpu_cycle(cpu);

    uint32_t expected = 0b10110;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("LSR") {
    cpu.set_register_value(PC, 0x6);

    // lsr rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 1);
    cpu_cycle(cpu);

    uint32_t expected = 0b0101;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("ASR") {
    cpu.set_register_value(PC, 0x8);

    // asr rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 1);
    cpu_cycle(cpu);

    uint32_t expected = 0b101;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("ADC") {
    cpu.set_register_value(PC, 0xA);

    // adc rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu.cspr |= CSPR_C;
    cpu_cycle(cpu);

    uint32_t expected = 0b1011 + 0b1101 + 1;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("SBC") {
    cpu.set_register_value(PC, 0xC);

    // sbc rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    // Rd := Rd - Rs - NOT(C)
    uint32_t expected = 0b1011 - 0b1101 - 1;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("ROR") {
    cpu.set_register_value(PC, 0xE);

    // ror rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 1);
    cpu_cycle(cpu);

    uint32_t expected = 0b101 | 1 << 31;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("TST") {
    cpu.set_register_value(PC, 0x10);

    // tst rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    bool result = cpu.cspr & CSPR_Z;
    REQUIRE_FALSE(result);

    cpu.set_register_value(PC, 0x10);

    // tst rd, rs
    cpu.set_register_value(0, 0b0000);
    cpu.set_register_value(1, 0b1011);
    cpu_cycle(cpu);

    result = cpu.cspr & CSPR_Z;
    REQUIRE(result);
  }

  SECTION("NEG") {
    cpu.set_register_value(PC, 0x12);

    // neg rd, rs
    cpu.set_register_value(1, 5);
    cpu_cycle(cpu);

    int32_t expected = -5;
    REQUIRE((int32_t)cpu.get_register_value(0) == expected);
  }

  SECTION("CMP") {
    cpu.set_register_value(PC, 0x14);

    // cmp rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    bool result = cpu.cspr & CSPR_Z;
    REQUIRE_FALSE(result);

    cpu.set_register_value(PC, 0x14);

    // cmp rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1011);
    cpu_cycle(cpu);

    result = cpu.cspr & CSPR_Z;
    REQUIRE(result);
  }

  SECTION("CMN") {
    cpu.set_register_value(PC, 0x16);

    // cmn r0, r1
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // 1 + 1 = 2
    bool result = cpu.cspr & CSPR_Z;
    REQUIRE_FALSE(result); // Z (Zero) flag should not be set.

    // cmn r0, r1
    cpu.set_register_value(PC, 0x16);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, (uint32_t)-1);
    cpu_cycle(cpu);

    // 1 + -1 = 0
    result = cpu.cspr & CSPR_Z;
    REQUIRE(result); // Z (Zero) flag should be set.
  }

  SECTION("ORR") {
    cpu.set_register_value(PC, 0x18);

    // orr rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = 0b1111;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("MUL") {
    cpu.set_register_value(PC, 0x1A);

    // mul rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = 0b1011 * 0b1101;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("BIC") {
    cpu.set_register_value(PC, 0x1C);

    // bic rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = 0b0010;
    REQUIRE(cpu.get_register_value(0) == expected);
  }

  SECTION("MVN") {
    cpu.set_register_value(PC, 0x1E);

    // mvn rd, rs
    cpu.set_register_value(0, 0b1011);
    cpu.set_register_value(1, 0b1101);
    cpu_cycle(cpu);

    uint32_t expected = ~0b1101;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}
