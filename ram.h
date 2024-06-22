#pragma once

#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <iomanip>
#include "memory_map.h"

#define MEMORY_MASK 0x0F000000
#define MEMORY_NOT_MASK 0x00FFFFFF

struct RAM {
  // BIOS - System ROM (16kb)
  // 0x00000000 - 0x00003FFF
  uint8_t* system_rom = new uint8_t[0x4000];

  // EWRAM - External Working RAM (256kb)
  // 0x02000000 - 0x0203FFFF
  uint8_t* external_working_ram = new uint8_t[0x40000];

  // IWRAM - Internal Working RAM (32kb)
  // 0x03000000 - 0x03007FFF
  uint8_t* internal_working_ram = new uint8_t[0x8000];

  // I/O Registers
  // 0x04000000 - 0x040003FE (but seems to be used beyond 0x04000400)
  uint8_t* io_registers = new uint8_t[0x804];

  // BG/OBJ Palette RAM (1kb)
  // 0x05000000 - 0x050003FF
  uint8_t* palette_ram = new uint8_t[0x400];

  // VRAM - Video RAM (96kb)
  // 0x06000000 - 0x06017FFF
  uint8_t* video_ram = new uint8_t[0x18000];

  // OAM - Object Attribute Memory (1kb)
  // 0x07000000 - 0x070003FF
  uint8_t* object_attribute_memory = new uint8_t[0x400];

  // Game Pak ROM/FlashROM (max 32MB) - Wait State 0
  // 0x08000000 - 0x09FFFFFF
  uint8_t* game_pak_rom = new uint8_t[0x2000000];

  std::map<uint32_t, uint8_t*> memory_map = {
    {BIOS_START,                 system_rom},
    {WORKING_RAM_ON_BOARD_START, external_working_ram},
    {WORKING_RAM_ON_CHIP_START,  internal_working_ram},
    {IO_REGISTERS_START,         io_registers},
    {PALETTE_RAM_START,          palette_ram},
    {VRAM_START,                 video_ram},
    {OAM_START,                  object_attribute_memory},
    {GAME_PAK_ROM_START,         game_pak_rom}
  };
  std::map<uint32_t, uint32_t> memory_size = {
    {BIOS_START,                 0x4000},
    {WORKING_RAM_ON_BOARD_START, 0x40000},
    {WORKING_RAM_ON_CHIP_START,  0x8000},
    {IO_REGISTERS_START,         0x804},
    {PALETTE_RAM_START,          0x400},
    {VRAM_START,                 0x18000},
    {OAM_START,                  0x400},
    {GAME_PAK_ROM_START,         0x2000000}
  };
};

void ram_load_rom(RAM& ram, std::string const& path);
void ram_load_bios(RAM& ram, std::string const& path);

// swaps a 16-bit value
static inline uint16_t swap16(uint16_t v)
{
  return (v << 8) | (v >> 8);
}

// swaps a 32-bit value
static inline uint32_t swap32(uint32_t v)
{
  return (v << 24) | ((v << 8) & 0xff0000) | ((v >> 8) & 0xff00) | (v >> 24);
}

inline uint8_t* resolve_address(RAM& ram, uint32_t address) {
  uint32_t memory_loc = address & MEMORY_MASK;
  uint32_t offset = address & MEMORY_NOT_MASK;
  if (ram.memory_map.find(memory_loc) == ram.memory_map.end()) {
    std::stringstream ss;
    ss << "Error: Invalid memory location 0x" << std::hex << address;
    throw std::runtime_error("Error: Invalid memory location " + ss.str());
  }
  if (offset >= ram.memory_size[memory_loc]) {
    std::stringstream ss;
    ss << "Error: Out of bounds memory access 0x" << std::hex << address;
    throw std::runtime_error("Error: Out of bounds memory access " + ss.str());
  }

  return &ram.memory_map[memory_loc][offset];
}

inline uint8_t ram_read_byte(RAM& ram, uint32_t address) {
  return *resolve_address(ram, address);
}

inline uint16_t ram_read_half_word(RAM& ram, uint32_t address) {
  return *(uint16_t*)resolve_address(ram, address);
}

inline uint32_t ram_read_word(RAM& ram, uint32_t address) {
  return *(uint32_t*)resolve_address(ram, address);
}

inline int8_t ram_read_byte_signed(RAM& ram, uint32_t address) {
  return (int8_t)*resolve_address(ram, address);
}

inline int16_t ram_read_half_word_signed(RAM& ram, uint32_t address) {
  return (int16_t)ram_read_half_word(ram, address);
}

inline int32_t ram_read_word_signed(RAM& ram, uint32_t address) {
  return (int32_t)ram_read_word(ram, address);
}

inline void ram_write_byte(RAM& ram, uint32_t address, uint8_t value) {
  *resolve_address(ram, address) = value;
}

inline void ram_write_half_word(RAM& ram, uint32_t address, uint16_t value) {
  *(uint16_t*)resolve_address(ram, address) = value;
}

inline void ram_write_word(RAM& ram, uint32_t address, uint32_t value) {
  *(uint32_t*)resolve_address(ram, address) = value;
}
