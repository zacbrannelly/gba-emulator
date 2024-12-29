#include "flash.h"
#include "cpu.h"

#include <iostream>
#include <cstring>

void flash_init(CPU& cpu) {
  memset(cpu.ram.game_pak_sram, 0xFF, 0x20000);
  memset(cpu.flash.command_buffer, 0, 3);
  cpu.flash.command_buffer_index = 0;
  cpu.flash.mode = FlashMode::READ;
  cpu.flash.bank = 0;
}

void flash_write_byte(CPU& cpu, uint32_t address, uint8_t value) {
  if (address == 0xE005555) {
    auto command_idx = cpu.flash.command_buffer_index;
    cpu.flash.command_buffer[command_idx] = value;

    // Set the mode of the controller.
    if (cpu.flash.command_buffer[2] == 0x55 && command_idx == 1) {
      // Reset the command buffer.
      cpu.flash.command_buffer_index = 0;
      memset(cpu.flash.command_buffer, 0, 3);

      if (value == 0x90) {
        // Enter ID mode.
        cpu.flash.mode = FlashMode::ID_MODE;
        return;
      } else if (value == 0xF0) {
        // Terminate any mode.
        cpu.flash.mode = FlashMode::READ;
        return;
      } else if (value == 0x80) {
        // Enter erase mode.
        cpu.flash.mode = FlashMode::ERASE_MODE;
        return;
      } else if (value == 0x10) {
        // Erase the entire chip.
        memset(cpu.ram.game_pak_sram, 0xFF, 0x20000);
        cpu.flash.mode = FlashMode::READ;
        return;
      } else if (value == 0xA0) {
        // Enter write mode.
        cpu.flash.mode = FlashMode::WRITE_MODE;
        return;
      } else if (value == 0xB0) {
        // Enter select bank mode.
        cpu.flash.mode = FlashMode::SELECT_BANK_MODE;
        return;
      }
    }

    // Make sure the command buffer is filled with the correct values.
    // Otherwise ignore the data.
    if (cpu.flash.command_buffer[0] == 0xAA) {
      cpu.flash.command_buffer_index++;
      return;
    }
  } else if (cpu.flash.command_buffer[0] == 0xAA && address == 0xE002AAA) {
    cpu.flash.command_buffer[2] = value;
    return;
  }

  if (cpu.flash.mode == FlashMode::WRITE_MODE) {
    // WRITE mode
    int offset = address - GAME_PAK_SRAM_START + cpu.flash.bank * 0x10000;
    cpu.ram.game_pak_sram[offset] = value;
  } else if (cpu.flash.mode == FlashMode::ERASE_MODE) {
    // ERASE mode
    if (
      cpu.flash.command_buffer[0] == 0xAA &&
      cpu.flash.command_buffer[2] == 0x55
    ) {
      if (value == 0x30) {
        // Erase a sector.
        int offset = address - GAME_PAK_SRAM_START + cpu.flash.bank * 0x10000;
        memset(cpu.ram.game_pak_sram + offset, 0xFF, 0x1000);
      }
    }
  } else if (cpu.flash.mode == FlashMode::SELECT_BANK_MODE) {
    // SELECT BANK mode
    cpu.flash.bank = value;
  }
}

uint8_t flash_read_byte(CPU& cpu, uint32_t address) {
  if (cpu.flash.mode == FlashMode::ID_MODE) {
    // TODO: Figure out the ID of the flash chip.
    // For now, return a value that works for Pokemon Emerald.
    if (address == GAME_PAK_SRAM_START) {
      return 0x62;
    } else if (address == GAME_PAK_SRAM_START + 1 ) {
      return 0x13;
    }
  }

  // READ mode
  int offset = address - GAME_PAK_SRAM_START + cpu.flash.bank * 0x10000;
  return cpu.ram.game_pak_sram[offset];
}
