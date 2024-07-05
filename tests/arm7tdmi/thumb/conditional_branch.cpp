#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Conditional Branch", "[thumb, conditional-branch]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.load_rom_into_bios = true;
  cpu.ram.enable_rom_write_protection = false;

  REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/thumb/conditional_branch.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("BEQ") {
    cpu.set_register_value(PC, 0x0);

    // beq to instruction at 28
    // EQ (Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 2);

    cpu.set_register_value(PC, 0x0);

    // beq to instruction at 28
    // EQ (Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BNE") {
    cpu.set_register_value(PC, 0x2);

    // bne to instruction at 28
    // NE (Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x2);

    // bne to instruction at 28
    // NE (Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 4);
  }

  SECTION("BCS") {
    cpu.set_register_value(PC, 0x4);

    // bcs to instruction at 28
    // CS (C == 1)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 6);

    cpu.set_register_value(PC, 0x4);

    // bcs to instruction at 28
    // CS (C == 1)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BCC") {
    cpu.set_register_value(PC, 0x6);

    // bcc to instruction at 28
    // CC (C == 0)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x6);

    // bcc to instruction at 28
    // CC (C == 0)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 8);
  }

  SECTION("BMI") {
    cpu.set_register_value(PC, 0x8);

    // bmi to instruction at 28
    // MI (N == 1)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 10);

    cpu.set_register_value(PC, 0x8);

    // bmi to instruction at 28
    // MI (N == 1)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BPL") {
    cpu.set_register_value(PC, 0xA);

    // bpl to instruction at 28
    // PL (N == 0)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0xA);

    // bpl to instruction at 28
    // PL (N == 0)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 12);
  }

  SECTION("BVS") {
    cpu.set_register_value(PC, 0xC);

    // bvs to instruction at 28
    // VS (V == 1)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 14);

    cpu.set_register_value(PC, 0xC);

    // bvs to instruction at 28
    // VS (V == 1)
    cpu.cspr = (uint32_t)User | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BVC") {
    cpu.set_register_value(PC, 0xE);

    // bvc to instruction at 28
    // VC (V == 0)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0xE);

    // bvc to instruction at 28
    // VC (V == 0)
    cpu.cspr = (uint32_t)User | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 16);
  }

  SECTION("BHI") {
    cpu.set_register_value(PC, 0x10);

    // bhi to instruction at 28
    // HI (C == 1 && Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 18);

    cpu.set_register_value(PC, 0x10);

    // bhi to instruction at 28
    // HI (C == 1 && Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x10);

    // bhi to instruction at 28
    // HI (C == 1 && Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 18);

    cpu.set_register_value(PC, 0x10);

    // bhi to instruction at 28
    // HI (C == 1 && Z == 0)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 18);
  }

  SECTION("BLS") {
    cpu.set_register_value(PC, 0x12);

    // bls to instruction at 28
    // LS (C == 0 || Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x12);

    // bls to instruction at 28
    // LS (C == 0 || Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 20);

    cpu.set_register_value(PC, 0x12);

    // bls to instruction at 28
    // LS (C == 0 || Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x12);

    // bls to instruction at 28
    // LS (C == 0 || Z == 1)
    cpu.cspr = (uint32_t)User | CSPR_C | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BGE") {
    cpu.set_register_value(PC, 0x14);

    // bge to instruction at 28
    // GE (N == V)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x14);

    // bge to instruction at 28
    // GE (N == V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x14);

    // bge to instruction at 28
    // GE (N == V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 22);

    cpu.set_register_value(PC, 0x14);

    // bge to instruction at 28
    // GE (N == V)
    cpu.cspr = (uint32_t)User | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 22);
  }

  SECTION("BLT") {
    cpu.set_register_value(PC, 0x16);

    // blt to instruction at 28
    // LT (N != V)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 24);

    cpu.set_register_value(PC, 0x16);

    // blt to instruction at 28
    // LT (N != V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 24);

    cpu.set_register_value(PC, 0x16);

    // blt to instruction at 28
    // LT (N != V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x16);

    // blt to instruction at 28
    // LT (N != V)
    cpu.cspr = (uint32_t)User | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }

  SECTION("BGT") {
    cpu.set_register_value(PC, 0x18);

    // bgt to instruction at 28
    // GT (!Z && N == V)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x18);

    // bgt to instruction at 28
    // GT (!Z && N == V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x18);

    // bgt to instruction at 28
    // GT (!Z && N == V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 26);

    cpu.set_register_value(PC, 0x18);

    // bgt to instruction at 28
    // GT (!Z && N == V)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 26);

    cpu.set_register_value(PC, 0x18);

    // bgt to instruction at 28
    // GT (!Z && N == V)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 26);
  }

  SECTION("BLE") {
    cpu.set_register_value(PC, 0x1A);

    // ble to instruction at 28
    // LE (Z || N != V)
    cpu.cspr = (uint32_t)User | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x1C);

    cpu.set_register_value(PC, 0x1A);

    // ble to instruction at 28
    // LE (Z || N != V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x1C);

    cpu.set_register_value(PC, 0x1A);

    // ble to instruction at 28
    // LE (Z || N != V)
    cpu.cspr = (uint32_t)User | CSPR_N | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x1A);

    // ble to instruction at 28
    // LE (Z || N != V)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);

    cpu.set_register_value(PC, 0x1A);

    // ble to instruction at 28
    // LE (Z || N != V)
    cpu.cspr = (uint32_t)User | CSPR_Z | CSPR_N | CSPR_V | CSPR_THUMB_STATE;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 28);
  }
}
