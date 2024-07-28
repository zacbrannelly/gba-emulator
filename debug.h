#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cpu.h>

inline void debug_print_cpu_state(CPU& cpu) {
  std::cout << "PC: " << std::hex << cpu.get_register_value(PC) << std::endl;

  for (int i = 0; i < 16; i++) {
    std::cout << "R" << std::dec << i << ": " << std::hex << cpu.get_register_value(i) << std::endl;
  }
  std::cout << "CPSR: " << std::hex << cpu.cpsr << std::endl;

  if (cpu.cpsr & CPSR_THUMB_STATE) {
    std::cout << "Instruction: " << std::hex << ram_read_half_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
  } else {
    std::cout << "Instruction: " << std::hex << ram_read_word(cpu.ram, cpu.get_register_value(PC)) << std::endl;
  }

  uint16_t reg_interrupt_enable = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_ENABLE>(cpu.ram);
  uint16_t reg_interrupt_request_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
  uint16_t reg_interrupt_master_enable = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_MASTER_ENABLE>(cpu.ram);
  std::cout << "Interrupt Enable: " << std::hex << reg_interrupt_enable << std::endl;
  std::cout << "Interrupt Request Flags: " << std::hex << reg_interrupt_request_flags << std::endl;
  std::cout << "Interrupt Master Enable: " << std::hex << reg_interrupt_master_enable << std::endl;

  std::cout << "Cycle Count: " << std::dec << cpu.cycle_count << std::endl;

  std::cout << std::endl;
}

inline void print_screen_state(CPU& cpu) {
  std::cout << std::hex << std::setfill('0');

  // LCD Control
  uint16_t lcd_control = ram_read_word(cpu.ram, REG_LCD_CONTROL);
  std::cout << "LCD Control: 0x" << std::setw(4) << lcd_control << std::endl;
  std::cout << "  Mode: " << (lcd_control & 0x7) << std::endl;
  std::cout << "  BG0 Enable: " << ((lcd_control >> 8) & 1) << std::endl;
  std::cout << "  BG1 Enable: " << ((lcd_control >> 9) & 1) << std::endl;
  std::cout << "  BG2 Enable: " << ((lcd_control >> 10) & 1) << std::endl;
  std::cout << "  BG3 Enable: " << ((lcd_control >> 11) & 1) << std::endl;
  std::cout << "  OBJ Enable: " << ((lcd_control >> 12) & 1) << std::endl;

  // LCD Status
  uint16_t lcd_status = ram_read_word(cpu.ram, REG_LCD_STATUS);
  std::cout << "LCD Status: 0x" << std::setw(4) << lcd_status << std::endl;
  std::cout << "  V-Blank Flag: " << (lcd_status & 1) << std::endl;
  std::cout << "  H-Blank Flag: " << ((lcd_status >> 1) & 1) << std::endl;
  std::cout << "  V-Count Flag: " << ((lcd_status >> 2) & 1) << std::endl;

  // Vertical Count
  uint16_t vcount = ram_read_word(cpu.ram, REG_VERTICAL_COUNT);
  std::cout << "Vertical Count: " << std::dec << vcount << std::endl;

  // Background Control
  for (int i = 0; i < 4; i++) {
    uint16_t bg_control = ram_read_word(cpu.ram, REG_BG0_CONTROL + i * 2);
    std::cout << "BG" << i << " Control: 0x" << std::hex << std::setw(4) << bg_control << std::endl;
    std::cout << "  Priority: " << (bg_control & 3) << std::endl;
    std::cout << "  Character Base Block: " << ((bg_control >> 2) & 3) << std::endl;
    std::cout << "  Mosaic: " << ((bg_control >> 6) & 1) << std::endl;
    std::cout << "  Color Mode: " << (((bg_control >> 7) & 1) ? "256 colors" : "16 colors") << std::endl;
    std::cout << "  Screen Base Block: " << ((bg_control >> 8) & 0x1F) << std::endl;
    std::cout << "  Screen Size: " << ((bg_control >> 14) & 3) << std::endl;
  }

  // Window settings
  uint16_t win_in = ram_read_word(cpu.ram, REG_WINDOW_INSIDE);
  uint16_t win_out = ram_read_word(cpu.ram, REG_WINDOW_OUTSIDE);
  std::cout << "Window Inside: 0x" << std::setw(4) << win_in << std::endl;
  std::cout << "Window Outside: 0x" << std::setw(4) << win_out << std::endl;

  // Mosaic
  uint16_t mosaic = ram_read_word(cpu.ram, REG_MOSAIC_SIZE);
  std::cout << "Mosaic Size: 0x" << std::setw(4) << mosaic << std::endl;

  std::cout << std::endl; // Add a newline for readability between frames
}
