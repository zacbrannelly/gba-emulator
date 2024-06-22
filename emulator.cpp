#include <fstream>
#include <string>
#include <iostream>
#include <bitset>

#include "debug.h"
#include "cpu.h"

void cycle(CPU& cpu) {
  std::cout << "Instruction Count: " << std::dec << cpu.cycle_count++ << std::endl;
  std::cout << "PC: " << std::hex << cpu.get_register_value(PC) << std::endl;
  cpu_cycle(cpu);
  debug_print_cpu_state(cpu);
}

uint32_t parse_hex_to_int(std::string& hex) {
  uint32_t addr = 0;
  std::stringstream ss;
  ss << std::hex << hex;
  ss >> addr;
  return addr;
}

void emulator_loop(CPU& cpu) {
  cpu_init(cpu);
  ram_load_bios(cpu.ram, "gba_bios.bin");
  ram_load_rom(cpu.ram, "pokemon_emerald.gba");

  // Start from the beginning of the ROM.
  cpu.set_register_value(PC, GAME_PAK_ROM_START);

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
    cycle(cpu);

    // Press Enter to continue.
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "q") {
      break;
    } else if (input.size() > 0 && input.at(0) == 's') {
      // skip n instructions
      int n = std::stoi(input.substr(2));
      for (int i = 0; i < n; i++) {
        cycle(cpu);
      }
    } else if (input.size() > 0 && input.at(0) == 't') {
      std::string hex = input.substr(2);
      uint32_t addr = parse_hex_to_int(hex);

      while (cpu.get_register_value(PC) != addr) {
        cycle(cpu);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  CPU cpu;

  try {
    emulator_loop(cpu);
  } catch (std::exception& e) {
    debug_print_cpu_state(cpu);
    std::cout << e.what() << std::endl;
  }

  return 0;
}
