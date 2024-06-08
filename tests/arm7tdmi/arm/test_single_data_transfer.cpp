#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Single Data Transfer", "[arm, single-data-transfer]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  SECTION("LDR") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_single_data_transfer.bin"));
    cpu.registers[PC] = 0x0;

    // ldr r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    *(uint32_t*)&cpu.memory[0x64] = 0x12121212;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);

    // ldr r0, [r1, #4]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    *(uint32_t*)&cpu.memory[0x68] = 0x34343434;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x34343434);

    // ldr r0, [r1, r2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    *(uint32_t*)&cpu.memory[0x68] = 0x56565656;
    cpu_cycle(cpu);
    
    REQUIRE(cpu.get_register_value(0) == 0x56565656);

    // ldr r0, [r1, r2, lsl #2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    *(uint32_t*)&cpu.memory[0x68] = 0x78787878;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x78787878);

    // ldr r0, [r1, r2, lsl #2]!
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    *(uint32_t*)&cpu.memory[0x68] = 0x78787878;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x78787878);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldr r0, [r1], r2
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    *(uint32_t*)&cpu.memory[0x64] = 0x12121212;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldr r0, [r1], r2, lsl #2
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    *(uint32_t*)&cpu.memory[0x64] = 0x12121212;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // ldrb r0, [r1]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x64);
    cpu.memory[0x64] = 0x12;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12);
  }

  SECTION("STR") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_single_data_transfer.bin"));
    cpu.registers[PC] = 0x20;

    // str r0, [r1]
    *(uint32_t*)&cpu.memory[0x64] = 0x0;
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x64] == 0x12121212);

    // str r0, [r1, #4]
    *(uint32_t*)&cpu.memory[0x68] = 0x0;
    cpu.set_register_value(0, 0x34343434);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x68] == 0x34343434);

    // str r0, [r1, r2]
    *(uint32_t*)&cpu.memory[0x68] = 0x0;
    cpu.set_register_value(0, 0x56565656);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x68] == 0x56565656);

    // str r0, [r1, r2, lsl #2]
    *(uint32_t*)&cpu.memory[0x68] = 0x0;
    cpu.set_register_value(0, 0x78787878);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x68] == 0x78787878);

    // str r0, [r1, r2, lsl #2]!
    *(uint32_t*)&cpu.memory[0x68] = 0x0;
    cpu.set_register_value(0, 0x78787878);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x68] == 0x78787878);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // str r0, [r1], r2
    *(uint32_t*)&cpu.memory[0x64] = 0x0;
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x4);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x64] == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // str r0, [r1], r2, lsl #2
    *(uint32_t*)&cpu.memory[0x64] = 0x0;
    cpu.set_register_value(0, 0x12121212);
    cpu.set_register_value(1, 0x64);
    cpu.set_register_value(2, 0x1);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x64] == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x68);

    // strb r0, [r1]
    *(uint32_t*)&cpu.memory[0x64] = 0x0;
    cpu.set_register_value(0, 0x12);
    cpu.set_register_value(1, 0x64);
    cpu_cycle(cpu);

    REQUIRE(cpu.memory[0x64] == 0x12);
  }
}
