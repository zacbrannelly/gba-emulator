#include <fstream>
#include <string>
#include <iostream>
#include <bitset>
#include <chrono>

#include "debug.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "timer.h"

void cycle(CPU& cpu, Timer& timer) {
  // PC alignment check.
  if (cpu.cspr & CSPR_THUMB_STATE) {
    if (cpu.get_register_value(PC) % 2 != 0) {
      throw std::runtime_error("PC is not aligned to 2 bytes.");
    }
  } else {
    if (cpu.get_register_value(PC) % 4 != 0) {
      throw std::runtime_error("PC is not aligned to 4 bytes.");
    }
  }

  cpu_cycle(cpu);
  cpu_interrupt_cycle(cpu);
  
  // Print the CPU State for debugging.
  // TODO: Disable with a flag.
  // debug_print_cpu_state(cpu);

  // Cycle the GPU.
  gpu_cycle(cpu);

  // Cycle the DMA controller.
  dma_cycle(cpu);

  // Cycle the Timer controller.
  timer_tick(cpu, timer);

  cpu.cycle_count++;
}

uint32_t parse_hex_to_int(std::string& hex) {
  uint32_t addr = 0;
  std::stringstream ss;
  ss << std::hex << hex;
  ss >> addr;
  return addr;
}

void emulator_loop(CPU& cpu, Timer& timer) {
  cpu_init(cpu);
  timer_init(cpu, timer);
  
  ram_load_bios(cpu.ram, "gba_bios.bin");
  ram_load_rom(cpu.ram, "pokemon_emerald.gba");

  // Start from the beginning of the ROM.
  // cpu.set_register_value(PC, GAME_PAK_ROM_START);
  cpu.set_register_value(PC, 0x0);

  // Set the SP to the end of the working RAM on-chip (matching VirtualBoy Advance).
  cpu.set_register_value(SP, WORKING_RAM_ON_CHIP_END - 0xFF);

  // Set the SP for each mode.
  // TODO: Clean this up and move it to a function (perhaps cpu_init?).
  uint32_t cspr_backup = cpu.cspr;
  cpu.cspr = (uint32_t)Supervisor;
  cpu.set_register_value(SP, 0x3007FE0);

  cpu.cspr = (uint32_t)IRQ;
  cpu.set_register_value(SP, 0x3007FA0);

  cpu.cspr = (uint32_t)System;
  cpu.set_register_value(SP, 0x3007F00);

  // Reset the CSPR to the original value.
  cpu.cspr = cspr_backup;

  while(true) {
    cycle(cpu, timer);
    debug_print_cpu_state(cpu);

    // Press Enter to continue.
    std::string input;
    std::getline(std::cin, input);

    if (input == "q") {
      break;
    } else if (input.size() > 0 && input.at(0) == 's') {
      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

      // skip n instructions
      int n = std::stoi(input.substr(2));
      for (int i = 0; i < n; i++) {
        cycle(cpu, timer);
      }

      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - begin;
      std::cout << "Elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
      std::cout << "Instructions per second: " << n / elapsed_seconds.count() << std::endl;
    } else if (input.size() > 0 && input.at(0) == 't') {
      std::string hex = input.substr(2);
      uint32_t addr = parse_hex_to_int(hex);

      while (cpu.get_register_value(PC) != addr) {
        cycle(cpu, timer);
      }
    } else if (input.size() > 0 && input.at(0) == 'v') {
      uint8_t scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
      bool hit_vcount = false;
      while (scanline != 160) {
        cycle(cpu, timer);
        scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
      }
      while (scanline == 160) {
        cycle(cpu, timer);
        scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  CPU cpu;
  Timer timer;

  try {
    emulator_loop(cpu, timer);
  } catch (std::exception& e) {
    debug_print_cpu_state(cpu);
    std::cout << e.what() << std::endl;
  }

  return 0;
}
