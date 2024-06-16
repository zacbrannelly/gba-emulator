#include <fstream>
#include <string>
#include <iostream>
#include <bitset>

#include "cpu.h"

int main(int argc, char* argv[]) {
  CPU cpu;
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

  int instruction_count = 0;

  while(true) {
    std::cout << "Instruction Count: " << std::dec << instruction_count++ << std::endl;
    std::cout << "PC: " << std::hex << cpu.get_register_value(PC) << std::endl;
    for (int i = 0; i < 16; i++) {
      std::cout << "R" << std::dec << i << ": " << std::hex << cpu.get_register_value(i) << std::endl;
    }
    std::cout << "CSPR: " << std::hex << cpu.cspr << std::endl;
    std::cout << std::endl;

    try {
      cpu_cycle(cpu);
    } catch (std::exception& e) {
      std::cout << e.what() << std::endl;
      if (cpu.cspr & CSPR_THUMB_STATE) {
        std::cout << "Instruction: " << std::hex << ram_read_half_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
      } else {
        std::cout << "Instruction: " << std::hex << ram_read_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
      }
      break;
    }

    // Press Enter to continue.
    std::string input;
    std::getline(std::cin, input);
    if (input == "q") {
      break;
    }
  }

  return 0;
}
