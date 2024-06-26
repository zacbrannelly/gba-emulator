#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Data Processing", "[arm, data-processing]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;


  SECTION("MOV") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/mov.bin"));
    cpu.registers[PC] = 0x0;
    for (int i = 0; i < 4; i++) {
      cpu.registers[i] = 0x0;
    }

    // Test immediate values.
    for (int i = 0; i < 4; i++) {
      // mov rn, #(n + 1)
      cpu_cycle(cpu);
      REQUIRE(cpu.registers[i] == (uint32_t)(i + 1));
      REQUIRE(cpu.registers[PC] == (i + 1) * 4);
    }

    // Test register value.
    // R4 = R3
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[4] == cpu.registers[3]);

    // mov r5, r2, lsl r1
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == cpu.registers[2] << cpu.registers[1]);

    // mov r5, r2, lsl #1
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == cpu.registers[2] << 1);

    // mov r5, r2, lsr #1
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == cpu.registers[2] >> 1);

    // mov r5, r2, asr #1
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == (int32_t)cpu.registers[2] >> 1);

    // mov r5, r2, ror #1
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == 0x80000001);

    // mov r5, r2, rrx
    // Set the carry flag.
    cpu.cspr |= 1 << 29;
    cpu_cycle(cpu);
    REQUIRE(cpu.registers[5] == 0x80000001);
  }

  SECTION("MOV to PC") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/mov_to_pc.bin"));
    // In supervisor mode, set the PC and LR.
    cpu.cspr = (uint32_t)Supervisor;
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(LR, 0x5);

    // Pretend there is some CPU state backed up.
    cpu.mode_to_scspr[Supervisor] = (uint32_t)User;

    // mov pc, lr
    cpu_cycle(cpu);

    // PC should be set to LR and mode should be restored.
    // PC should be set to 0x4 because the instruction should ensure the PC is at least 2 bytes aligned.
    REQUIRE(cpu.get_register_value(PC) == 0x4);
    REQUIRE(cpu.cspr == (uint32_t)User);
  }

  SECTION("AND") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/and.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // and r2, r0, #1
    cpu.set_register_value(0, 0x1);
    cpu_cycle(cpu);

    // R2 should be true (1).
    REQUIRE(cpu.get_register_value(2) == 0x1);

    // ands r2, r0, #1
    cpu.set_register_value(0, 0x0);
    cpu_cycle(cpu);

    // R2 should be false (0) and Z (Zero) flag should be set.
    REQUIRE(cpu.get_register_value(2) == 0x0);
    REQUIRE(cpu.cspr & (1 << 30));
  }

  SECTION("EOR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/eor.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // eor r2, r0, #1
    cpu.set_register_value(0, 0x1);
    cpu_cycle(cpu);

    // R2 should be false (0).
    REQUIRE(cpu.get_register_value(2) == 0x0);

    // eor r2, r0, #1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x0);
    cpu_cycle(cpu);

    // R2 should be true (1).
    REQUIRE(cpu.get_register_value(2) == 0x1);
  }

  SECTION("SUB") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/sub.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // sub r2, r0, #1
    cpu.set_register_value(0, 0x1);
    cpu_cycle(cpu);

    // R2 should be false (0).
    REQUIRE(cpu.get_register_value(2) == 0x0);

    // subs r2, r0, #1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x0);
    cpu_cycle(cpu);

    // R2 should be true (1) and N (Negative) flag should be set.
    REQUIRE(cpu.get_register_value(2) == (int32_t)-1);
    REQUIRE(cpu.cspr & (1 << 31));
  }

  SECTION("RSB") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/rsb.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // rsb r2, r0, #1
    cpu.set_register_value(0, 0x1);
    cpu_cycle(cpu);

    // 1 - 1 = 0
    REQUIRE(cpu.get_register_value(2) == 0x0);

    // rsbs r2, r0, #1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x5);
    cpu_cycle(cpu);

    // 1 - 5 = 2
    REQUIRE(cpu.get_register_value(2) == (int32_t)-4);
  }

  SECTION("ADD") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/add.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // add r2, r0, #1
    cpu.set_register_value(0, 0x1);
    cpu_cycle(cpu);

    // 1 + 1 = 2
    REQUIRE(cpu.get_register_value(2) == 0x2);

    // adds r2, r0, #1
    cpu.set_register_value(0, (uint32_t)-5);
    cpu_cycle(cpu);

    // -5 + 1 = -4
    REQUIRE(cpu.get_register_value(2) == (int32_t)-4);
    REQUIRE(cpu.cspr & (1 << 31)); // N (Negative) flag should be set.
  }

  SECTION("ADC") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/adc.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // adc r0, r1, r2
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    // 1 + 1 + 0 = 2
    REQUIRE(cpu.get_register_value(0) == 0x2);

    // adcs r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu.cspr = (uint32_t)User;
    cpu.cspr |= 1 << 29; // Set the carry flag.
    cpu_cycle(cpu);

    // 1 + 1 + 1 = 3
    REQUIRE(cpu.get_register_value(0) == 0x3);
  }

  SECTION("SBC") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/sbc.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // sbc r0, r1, r2
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    // 1 - 1 + 0 - 1 = -1
    REQUIRE(cpu.get_register_value(0) == (int32_t)-1);

    // sbcs r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu.cspr = (uint32_t)User;
    cpu.cspr |= 1 << 29; // Set the carry flag.
    cpu_cycle(cpu);

    // 1 - 1 + 1 - 1 = 0
    REQUIRE(cpu.get_register_value(0) == 0x0);
  }

  SECTION("RSC") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/rsc.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // rsc r0, r1, r2
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x2);
    cpu_cycle(cpu);

    // 2 - 1 + 0 - 1 = 0
    REQUIRE(cpu.get_register_value(0) == 0x0);

    // rsc r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x2);
    cpu.cspr = (uint32_t)User;
    cpu.cspr |= 1 << 29; // Set the carry flag.
    cpu_cycle(cpu);

    // 2 - 1 + 1 - 1 = 1
    REQUIRE(cpu.get_register_value(0) == 0x1);
  }

  SECTION("TST") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/tst.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // tst r0, r1
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // 1 & 1 = 1
    REQUIRE_FALSE(cpu.cspr & (1 << 30)); // Z (Zero) flag should not be set.

    // tst r0, r1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x0);
    cpu_cycle(cpu);

    // 1 & 0 = 0
    REQUIRE(cpu.cspr & (1 << 30)); // Z (Zero) flag should be set.
  }

  SECTION("TEQ") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/teq.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // teq r0, r1
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // 1 ^ 1 = 0
    REQUIRE(cpu.cspr & (1 << 30)); // Z (Zero) flag should be set.

    // teq r0, r1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x0);
    cpu_cycle(cpu);

    // 1 ^ 0 = 1
    REQUIRE_FALSE(cpu.cspr & (1 << 30)); // Z (Zero) flag should not be set.
  }

  SECTION("CMP") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/cmp.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // cmp r0, r1
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // 1 - 1 = 0
    REQUIRE(cpu.cspr & (1 << 30)); // Z (Zero) flag should be set.

    // cmp r0, r1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x0);
    cpu_cycle(cpu);

    // 1 - 0 = 1
    REQUIRE_FALSE(cpu.cspr & (1 << 30)); // Z (Zero) flag should not be set.
  }

  SECTION("CMN") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/cmn.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // cmn r0, r1
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // 1 + 1 = 2
    REQUIRE_FALSE(cpu.cspr & CSPR_Z); // Z (Zero) flag should not be set.

    // cmn r0, r1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(0, 0x1);
    cpu.set_register_value(1, (uint32_t)-1);
    cpu_cycle(cpu);

    // 1 + -1 = 0
    REQUIRE(cpu.cspr & (1 << 30)); // Z (Zero) flag should be set.
  }

  SECTION("ORR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/orr.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // orr r0, r1, r2
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    // 1 | 1 = 1
    REQUIRE(cpu.get_register_value(0) == 0x1);

    // orr r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x0);
    cpu_cycle(cpu);

    // 1 | 0 = 1
    REQUIRE(cpu.get_register_value(0) == 0x1);

    // orr r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu_cycle(cpu);

    // 0 | 0 = 0
    REQUIRE(cpu.get_register_value(0) == 0x0);
  }

  SECTION("BIC") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/bic.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // bic r0, r1, r2
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    // 1 & ~1 = 0
    REQUIRE(cpu.get_register_value(0) == 0x0);

    // bic r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x0);
    cpu_cycle(cpu);

    // 1 & ~0 = 1
    REQUIRE(cpu.get_register_value(0) == 0x1);

    // bic r0, r1, r2
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu_cycle(cpu);

    // 0 & ~0 = 0
    REQUIRE(cpu.get_register_value(0) == 0x0);
  }

  SECTION("MVN") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/mvn.bin"));
    cpu.cspr = (uint32_t)User;
    cpu.set_register_value(PC, 0x0);

    // mvn r0, r1
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    // ~1 = 0xFFFFFFFE
    REQUIRE(cpu.get_register_value(0) == 0xFFFFFFFE);

    // mvn r0, r1
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(1, 0x0);
    cpu_cycle(cpu);

    // ~0 = 0xFFFFFFFF
    REQUIRE(cpu.get_register_value(0) == 0xFFFFFFFF);
  }

  SECTION("MRS") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/mrs.bin"));
    cpu.cspr = (uint32_t)Supervisor;
    cpu.set_register_value(PC, 0x0);

    // mrs r0, cpsr
    cpu_cycle(cpu);

    // CPSR should be copied to R0.
    REQUIRE(cpu.get_register_value(0) == cpu.cspr);

    // mrs r0, spsr
    cpu.mode_to_scspr[Supervisor] = 0x12345678;
    cpu_cycle(cpu);

    // SPSR should be copied to R0.
    REQUIRE(cpu.get_register_value(0) == 0x12345678);
  }

  SECTION("MSR") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/data_processing/msr.bin"));
    cpu.cspr = (uint32_t)Supervisor;
    cpu.set_register_value(PC, 0x0);

    // msr cpsr, r0
    cpu.set_register_value(0, (uint32_t)User);
    cpu_cycle(cpu);

    // R0 should be copied to CPSR.
    REQUIRE(cpu.cspr == (uint32_t)User);

    // msr spsr, r0
    cpu.cspr = (uint32_t)Supervisor;
    cpu.set_register_value(0, 0x87654321);
    cpu_cycle(cpu);

    // R0 should be copied to SPSR.
    REQUIRE(cpu.mode_to_scspr[Supervisor] == 0x87654321);
  }
}
