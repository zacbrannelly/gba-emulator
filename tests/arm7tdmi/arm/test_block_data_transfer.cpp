#include <catch_amalgamated.hpp>
#include <cstdint>
#include <cpu.h>

TEST_CASE("Block Data Transfer", "[arm, block-data-transfer]") {
  CPU cpu;
  REQUIRE_NOTHROW(cpu_init(cpu));

  // Map the GamePak ROM to 0x0 for these unit tests.
  cpu.ram.memory_map[0] = cpu.ram.game_pak_rom;


  SECTION("LDM") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_block_data_transfer.bin"));
    cpu.registers[PC] = 0x0;

    // ldmib r0, {r1, r2, r3} - Pre-increment load, load memory at r0 + 4 into r1, r2, and r3.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x68, 0x12121212);
    ram_write_word(cpu.ram, 0x6C, 0x34343434);
    ram_write_word(cpu.ram, 0x70, 0x56565656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x64);

    // ldmib r0!, {r1, r2, r3} - Pre-increment load, load memory at r0 + 4 into r1, r2, and r3 and increment r0 by 12.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x68, 0x12121212);
    ram_write_word(cpu.ram, 0x6C, 0x34343434);
    ram_write_word(cpu.ram, 0x70, 0x56565656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // ldmia r0, {r1, r2, r3} - Post-increment load, load memory at r0 into r1, r2, and r3.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x64, 0x12121212);
    ram_write_word(cpu.ram, 0x68, 0x34343434);
    ram_write_word(cpu.ram, 0x6C, 0x56565656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x64);

    // ldmia r0!, {r1, r2, r3} - Post-increment load, load memory at r0 into r1, r2, and r3 and increment r0 by 12.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x64, 0x12121212);
    ram_write_word(cpu.ram, 0x68, 0x34343434);
    ram_write_word(cpu.ram, 0x6C, 0x56565656);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // ldmdb r0, {r1, r2, r3} - Pre-decrement load, load memory at r0 - 4 into r1, r2, and r3.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x70 - 4, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 8, 0x34343434);
    ram_write_word(cpu.ram, 0x70 - 12, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // ldmdb r0!, {r1, r2, r3} - Pre-decrement load, load memory at r0 - 4 into r1, r2, and r3 and decrement r0 by 12.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x70 - 4, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 8, 0x34343434);
    ram_write_word(cpu.ram, 0x70 - 12, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x64);

    // ldmda r0, {r1, r2, r3} // Post-decrement load, load memory at r0 into r1, r2, and r3.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x70, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 4, 0x34343434);
    ram_write_word(cpu.ram, 0x70 - 8, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // ldmda r0!, {r1, r2, r3} - Post-decrement load, load memory at r0 into r1, r2, and r3 and decrement r0 by 12.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x0);
    cpu.set_register_value(2, 0x0);
    cpu.set_register_value(3, 0x0);
    ram_write_word(cpu.ram, 0x70, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 4, 0x34343434);
    ram_write_word(cpu.ram, 0x70 - 8, 0x12121212);
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(1) == 0x12121212);
    REQUIRE(cpu.get_register_value(2) == 0x34343434);
    REQUIRE(cpu.get_register_value(3) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70 - 12);
  }

  SECTION("STM") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_block_data_transfer.bin"));
    cpu.registers[PC] = 0x20;

    // stmib r0, {r1, r2, r3} - Pre-increment store, store r1, r2, and r3 into memory at r0 + 4.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x68, 0x0);
    ram_write_word(cpu.ram, 0x6C, 0x0);
    ram_write_word(cpu.ram, 0x70, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x12121212);
    REQUIRE(ram_read_word(cpu.ram, 0x6C) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70) == 0x56565656);

    // stmib r0!, {r1, r2, r3} - Pre-increment store, store r1, r2, and r3 into memory at r0 + 4 and increment r0 by 12.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x68, 0x0);
    ram_write_word(cpu.ram, 0x6C, 0x0);
    ram_write_word(cpu.ram, 0x70, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x12121212);
    REQUIRE(ram_read_word(cpu.ram, 0x6C) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // stmia r0, {r1, r2, r3} - Post-increment store, store r1, r2, and r3 into memory at r0.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x64, 0x0);
    ram_write_word(cpu.ram, 0x68, 0x0);
    ram_write_word(cpu.ram, 0x6C, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x64) == 0x12121212);
    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x6C) == 0x56565656);

    // stmia r0!, {r1, r2, r3} - Post-increment store, store r1, r2, and r3 into memory at r0 and increment r0 by 12.
    cpu.set_register_value(0, 0x64);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x64, 0x0);
    ram_write_word(cpu.ram, 0x68, 0x0);
    ram_write_word(cpu.ram, 0x6C, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x64) == 0x12121212);
    REQUIRE(ram_read_word(cpu.ram, 0x68) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x6C) == 0x56565656);
    REQUIRE(cpu.get_register_value(0) == 0x70);

    // stmdb r0, {r1, r2, r3} // Pre-decrement store, store r1, r2, and r3 into memory at r0 - 4.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 4, 0x0);
    ram_write_word(cpu.ram, 0x70 - 8, 0x0);
    ram_write_word(cpu.ram, 0x70 - 12, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x70 - 4) == 0x56565656);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 8) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 12) == 0x12121212);

    // stmdb r0!, {r1, r2, r3} - Pre-decrement store, store r1, r2, and r3 into memory at r0 - 4 and decrement r0 by 12.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x70 - 4, 0x0);
    ram_write_word(cpu.ram, 0x70 - 8, 0x0);
    ram_write_word(cpu.ram, 0x70 - 12, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x70 - 4) == 0x56565656);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 8) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 12) == 0x12121212);
    REQUIRE(cpu.get_register_value(0) == 0x64);

    // stmda r0, {r1, r2, r3} - Post-decrement store, store r1, r2, and r3 into memory at r0.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x70, 0x0);
    ram_write_word(cpu.ram, 0x70 - 4, 0x0);
    ram_write_word(cpu.ram, 0x70 - 8, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x70) == 0x56565656);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 4) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 8) == 0x12121212);

    // stmda r0!, {r1, r2, r3} - Post-decrement store, store r1, r2, and r3 into memory at r0 and decrement r0 by 12.
    cpu.set_register_value(0, 0x70);
    cpu.set_register_value(1, 0x12121212);
    cpu.set_register_value(2, 0x34343434);
    cpu.set_register_value(3, 0x56565656);
    ram_write_word(cpu.ram, 0x70, 0x0);
    ram_write_word(cpu.ram, 0x70 - 4, 0x0);
    ram_write_word(cpu.ram, 0x70 - 8, 0x0);
    cpu_cycle(cpu);

    REQUIRE(ram_read_word(cpu.ram, 0x70) == 0x56565656);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 4) == 0x34343434);
    REQUIRE(ram_read_word(cpu.ram, 0x70 - 8) == 0x12121212);
    REQUIRE(cpu.get_register_value(0) == 0x70 - 12);
  }

  SECTION("User bank transfer and Mode changes") {
    REQUIRE_NOTHROW(ram_load_rom(cpu.ram, "./tests/arm7tdmi/arm/test_block_data_transfer.bin"));

    // ldmfd sp!, {r15}^ - R15 <- (SP), CPSR <- SPSR_mode
    cpu.cspr = (uint32_t)Supervisor;
    cpu.set_register_value(PC, 0x40);
    cpu.set_register_value(SP, 0x100);
    ram_write_word(cpu.ram, 0x100, 0x10);
    cpu.mode_to_scspr[Supervisor] = (uint32_t)User;
    cpu_cycle(cpu);

    REQUIRE(cpu.get_register_value(PC) == 0x10);
    REQUIRE(cpu.cspr == (uint32_t)User);

    // stmfd r13, {r0-r14}^ - Save user mode regs on stack (allowed only in privileged modes).
    // Pre-decrement store, store r0-r14 into memory at r13.
    cpu.cspr = (uint32_t)Supervisor;
    cpu.registers[13] = 0x32; // User mode SP
    cpu.registers[14] = 0x11; // User mode LR
    cpu.set_register_value(PC, 0x44); // Supervisor mode PC
    cpu.set_register_value(SP, 0x100); // Supervisor mode SP
    cpu_cycle(cpu);
    
    // Check the user mode registers are saved on the stack.
    REQUIRE(ram_read_word(cpu.ram, 0x100 - 4) == 0x11);
    REQUIRE(ram_read_word(cpu.ram, 0x100 - 8) == 0x32);
  }
}
