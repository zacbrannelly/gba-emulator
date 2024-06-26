#pragma once

#include <iostream>
#include <sstream>
#include <cpu.h>

inline void debug_print_cpu_state(CPU& cpu) {
  std::cout << "PC: " << std::hex << cpu.get_register_value(PC) << std::endl;

  if (cpu.cspr & CSPR_THUMB_STATE) {
    if (cpu.get_register_value(PC) % 2 != 0) {
      throw std::runtime_error("PC is not aligned to 2 bytes.");
    }
  } else {
    if (cpu.get_register_value(PC) % 4 != 0) {
      throw std::runtime_error("PC is not aligned to 4 bytes.");
    }
  }

  for (int i = 0; i < 16; i++) {
    std::cout << "R" << std::dec << i << ": " << std::hex << cpu.get_register_value(i) << std::endl;
  }
  std::cout << "CSPR: " << std::hex << cpu.cspr << std::endl;

  if (cpu.cspr & CSPR_THUMB_STATE) {
    std::cout << "Instruction: " << std::hex << ram_read_half_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
  } else {
    std::cout << "Instruction: " << std::hex << ram_read_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
  }

  std::cout << std::endl;
}
