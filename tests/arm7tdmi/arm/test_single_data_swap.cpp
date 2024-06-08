#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Single Data Swap", "[arm, single-data-swap]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  SECTION("SWP/SWPB") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_single_data_swap.bin"));
    cpu.registers[PC] = 0x0;

    // swp r0, r1, [r2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x64);
    *(uint32_t*)&cpu.memory[0x64] = 0x12121212;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12121212);
    REQUIRE(cpu.get_register_value(1) == 0x1);
    REQUIRE(*(uint32_t*)&cpu.memory[0x64] == 0x1);

    // swpb r0, r1, [r2]
    cpu.set_register_value(0, 0x0);
    cpu.set_register_value(1, 0x1);
    cpu.set_register_value(2, 0x64);
    cpu.memory[0x64] = 0x12;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(0) == 0x12);
    REQUIRE(cpu.get_register_value(1) == 0x1);
    REQUIRE(cpu.memory[0x64] == 0x1);
  }
}
