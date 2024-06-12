#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Push/Pop Registers", "[thumb, push-pop-registers]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));
  REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/push_pop_registers.bin"));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("PUSH") {
    cpu.set_register_value(PC, 0x0);
    cpu.set_register_value(SP, 0x2000);
    
    // push {r0, r1, r2}
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x87654321);
    cpu.set_register_value(2, 0x11111111);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 4] == 0x12345678);
    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 8] == 0x87654321);
    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 12] == 0x11111111);
    REQUIRE(cpu.get_register_value(SP) == 0x2000 - 12);

    cpu.set_register_value(PC, 0x2);
    cpu.set_register_value(SP, 0x2000);

    // push {r0, r1, r2, lr}
    cpu.set_register_value(0, 0x12345678);
    cpu.set_register_value(1, 0x87654321);
    cpu.set_register_value(2, 0x11111111);
    cpu.set_register_value(LR, 0x2);
    cpu_cycle(cpu);

    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 4] == 0x12345678);
    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 8] == 0x87654321);
    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 12] == 0x11111111);
    REQUIRE(*(uint32_t*)&cpu.memory[0x2000 - 16] == 0x2);
    REQUIRE(cpu.get_register_value(SP) == 0x2000 - 16);
  }

  SECTION("POP") {
    cpu.set_register_value(PC, 0x4);
    cpu.set_register_value(SP, 0x2000 - 12);

    // pop {r0, r1, r2}
    *(uint32_t*)&cpu.memory[0x2000 - 4] = 0x12345678;
    *(uint32_t*)&cpu.memory[0x2000 - 8] = 0x87654321;
    *(uint32_t*)&cpu.memory[0x2000 - 12] = 0x11111111;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x11111111);
    REQUIRE(cpu.get_register_value(1) == 0x87654321);
    REQUIRE(cpu.get_register_value(2) == 0x12345678);
    REQUIRE(cpu.get_register_value(SP) == 0x2000);

    // pop {r0, r1, r2, pc}
    cpu.set_register_value(SP, 0x2000 - 12);
    *(uint32_t*)&cpu.memory[0x2000] = 0x2;
    *(uint32_t*)&cpu.memory[0x2000 - 4] = 0x12345678;
    *(uint32_t*)&cpu.memory[0x2000 - 8] = 0x87654321;
    *(uint32_t*)&cpu.memory[0x2000 - 12] = 0x11111111;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x11111111);
    REQUIRE(cpu.get_register_value(1) == 0x87654321);
    REQUIRE(cpu.get_register_value(2) == 0x12345678);
    REQUIRE(cpu.get_register_value(PC) == 0x2);
    REQUIRE(cpu.get_register_value(SP) == 0x2004);
  }
}
