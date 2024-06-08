#include <catch_amalgamated.hpp>
#include <cstdint>
#include <utils.h>

TEST_CASE("Software Interrupt", "[arm, software-interrupt]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  SECTION("SWI") {
    REQUIRE_NOTHROW(load_rom(cpu, "./tests/arm7tdmi/arm/test_software_interrupt.bin"));
    cpu.registers[PC] = 0x0;
    cpu.cspr = (uint32_t)User;

    // swi 1
    cpu_cycle(cpu);

    REQUIRE(cpu.registers[PC] == 0x8);
    REQUIRE(cpu.cspr == (uint32_t)Supervisor);
    REQUIRE(cpu.mode_to_scspr[Supervisor] == (uint32_t)User);
  }
}
