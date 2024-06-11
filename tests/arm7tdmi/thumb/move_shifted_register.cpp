#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Move Shifted Register", "[thumb, move-shifted-register]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Enter Thumb State
  cpu.cspr |= CSPR_THUMB_STATE;

  SECTION("MOV (shifted register)") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/thumb/move_shifted_register.bin"));
    cpu.registers[PC] = 0x0;

    // lsl r0, r1, #5
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 0x1);
    cpu_cycle(cpu);

    int expected = 1 << 5;
    REQUIRE(cpu.get_register_value(0) == expected);

    // lsr r0, r1, #5
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 1 << 5);
    cpu_cycle(cpu);

    expected = 1;
    REQUIRE(cpu.get_register_value(0) == expected);

    // asr r0, r1, #5
    cpu.set_register_value(0, 0);
    cpu.set_register_value(1, 1 << 5);
    cpu_cycle(cpu);

    expected = 1;
    REQUIRE(cpu.get_register_value(0) == expected);
  }
}