#pragma once

#include <iostream>
#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "memory_map.h"

#define MEMORY_MASK 0x0F000000
#define MEMORY_NOT_MASK 0x00FFFFFF

enum MemoryLocation {
  BIOS,
  WORKING_RAM_ON_BOARD,
  WORKING_RAM_ON_CHIP,
  IO_REGISTERS,
  PALETTE_RAM,
  VRAM,
  OAM,
  GAME_PAK_ROM
};

struct RAM {
  // BIOS - System ROM (16kb)
  // 0x00000000 - 0x00003FFF
  uint8_t* system_rom = new uint8_t[0x4000];

  // EWRAM - External Working RAM (256kb) - Mirrored
  // 0x02000000 - 0x0203FFFF
  uint8_t* external_working_ram = new uint8_t[0x40000];

  // IWRAM - Internal Working RAM (32kb) - Mirrored
  // 0x03000000 - 0x03007FFF
  uint8_t* internal_working_ram = new uint8_t[0x8000];

  // I/O Registers
  // 0x04000000 - 0x040003FE (but seems to be used beyond 0x04000400)
  uint8_t* io_registers = new uint8_t[0x804];

  // BG/OBJ Palette RAM (1kb) - Mirrored
  // 0x05000000 - 0x050003FF
  uint8_t* palette_ram = new uint8_t[0x400];

  // VRAM - Video RAM (96kb) - TODO: Mirror depending on mode
  // 0x06000000 - 0x06017FFF
  uint8_t* video_ram = new uint8_t[0x18000];

  // OAM - Object Attribute Memory (1kb) - Mirror
  // 0x07000000 - 0x070003FF
  uint8_t* object_attribute_memory = new uint8_t[0x400];

  // Game Pak ROM/FlashROM (max 32MB) - Mirrored (0x08000000, 0x0A000000, 0x0C000000)
  // 0x08000000 - 0x09FFFFFF
  uint8_t* game_pak_rom = new uint8_t[0x2000000];

  // TODO: Might not need this anymore.
  std::unordered_map<uint32_t, uint32_t> memory_size = {
    {BIOS_START,                 0x4000},
    {WORKING_RAM_ON_BOARD_START, 0x40000},
    {WORKING_RAM_ON_CHIP_START,  0x8000},
    {IO_REGISTERS_START,         0x804},
    {PALETTE_RAM_START,          0x400},
    {VRAM_START,                 0x18000},
    {OAM_START,                  0x400},
    {GAME_PAK_ROM_START,         0x2000000}
  };

  std::vector<uint32_t> memory_write_hook_addresses;
  std::vector<uint32_t> memory_read_hook_addresses;
  std::unordered_map<uint32_t, std::function<void(RAM&, uint32_t, uint32_t)>> memory_write_hooks;
  std::unordered_map<uint32_t, std::function<uint32_t(RAM&, uint32_t)>> memory_read_hooks;

  bool load_rom_into_bios = false;
  bool enable_rom_write_protection = true;
};

void ram_init(RAM& ram);
void ram_soft_reset(RAM& ram);
void ram_load_rom(RAM& ram, std::string const& path);
void ram_load_bios(RAM& ram, std::string const& path);
void ram_register_read_hook(RAM& ram, uint32_t address, std::function<uint32_t(RAM&, uint32_t)> const& hook);
void ram_register_write_hook(RAM& ram, uint32_t address, std::function<void(RAM&, uint32_t, uint32_t)> const& hook);

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

template<uint32_t Offset>
inline uint8_t ram_read_byte_from_io_registers_fast(RAM& ram) {
  constexpr uint32_t offset = Offset & MEMORY_NOT_MASK;
  return ram.io_registers[offset];
}

template<uint32_t Offset>
inline uint32_t ram_read_word_from_io_registers_fast(RAM& ram) {
  constexpr uint32_t offset = Offset & MEMORY_NOT_MASK;
  return *(uint32_t*)&ram.io_registers[offset];
}

template<uint32_t Offset>
inline uint16_t ram_read_half_word_from_io_registers_fast(RAM& ram) {
  constexpr uint32_t offset = Offset & MEMORY_NOT_MASK;
  return *(uint16_t*)&ram.io_registers[offset];
}

inline bool ram_address_has_read_hook(RAM& ram, uint32_t address) {
  return std::find(ram.memory_read_hook_addresses.begin(), ram.memory_read_hook_addresses.end(), address) != ram.memory_read_hook_addresses.end();
}

inline bool ram_address_has_write_hook(RAM& ram, uint32_t address) {
  return std::find(ram.memory_write_hook_addresses.begin(), ram.memory_write_hook_addresses.end(), address) != ram.memory_write_hook_addresses.end();
}

inline uint8_t* ram_resolve_address(RAM& ram, uint32_t address) {
  uint32_t memory_loc = address & MEMORY_MASK;
  uint32_t offset = address & MEMORY_NOT_MASK;

  uint8_t* memory = nullptr;
  uint32_t mirror_interval = 0;
  if (memory_loc == BIOS_START) {
    memory = ram.system_rom;
  } else if (memory_loc == WORKING_RAM_ON_BOARD_START) {
    memory = ram.external_working_ram;
    mirror_interval = 0x40000;
  } else if (memory_loc == WORKING_RAM_ON_CHIP_START) {
    memory = ram.internal_working_ram;
    mirror_interval = 0x8000;
  } else if (memory_loc == IO_REGISTERS_START) {
    memory = ram.io_registers;
  } else if (memory_loc == PALETTE_RAM_START) {
    memory = ram.palette_ram;
    mirror_interval = 0x400;
  } else if (memory_loc == VRAM_START) {
    memory = ram.video_ram;
  } else if (memory_loc == OAM_START) {
    memory = ram.object_attribute_memory;
  } else if (
    memory_loc == GAME_PAK_ROM_START ||
    memory_loc == GAME_PAK_ROM_START + 0x1000000 ||
    memory_loc == GAME_PAK_ROM_WS1_START ||
    memory_loc == GAME_PAK_ROM_WS1_START + 0x1000000 ||
    memory_loc == GAME_PAK_ROM_WS2_START ||
    memory_loc == GAME_PAK_ROM_WS2_START + 0x1000000
  ) {
    memory = ram.game_pak_rom;
    mirror_interval = 0x2000000;
  } 
  else if (
    memory_loc == GAME_PAK_ROM_START + 0x1000000 ||
    memory_loc == GAME_PAK_ROM_WS1_START + 0x1000000 ||
    memory_loc == GAME_PAK_ROM_WS2_START + 0x1000000
  ) {
    memory = ram.game_pak_rom;
    mirror_interval = 0x2000000;
    offset += 0x1000000;
  } else if (memory_loc == GAME_PAK_SRAM_START) {
    throw std::runtime_error("Error: SRAM not implemented yet");
  } else {
    std::stringstream ss;
    ss << "Error: Invalid memory location: 0x" << std::hex << std::setw(8) << std::setfill('0') << memory_loc << " at address 0x" << std::hex << std::setw(8) << std::setfill('0') << address;
    throw std::runtime_error(ss.str());
  }

  // Check if the memory location is mirrored.
  if (mirror_interval > 0 && offset >= mirror_interval) {
    // Adjust the offset to the mirrored memory location.
    offset %= mirror_interval;
  }

  return &memory[offset];
}

inline uint8_t ram_read_byte(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return (uint8_t)ram.memory_read_hooks[address](ram, address);
  }
  return *ram_resolve_address(ram, address);
}

inline uint8_t ram_read_byte_direct(RAM& ram, uint32_t address) {
  return *ram_resolve_address(ram, address);
}

inline uint16_t ram_read_half_word(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return (uint16_t)ram.memory_read_hooks[address](ram, address);
  }
  return *(uint16_t*)ram_resolve_address(ram, address);
}

inline uint16_t ram_read_half_word_direct(RAM& ram, uint32_t address) {
  return *(uint16_t*)ram_resolve_address(ram, address);
}

inline uint32_t ram_read_word(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return ram.memory_read_hooks[address](ram, address);
  }
  return *(uint32_t*)ram_resolve_address(ram, address);
}

inline uint32_t ram_read_word_direct(RAM& ram, uint32_t address) {
  return *(uint32_t*)ram_resolve_address(ram, address);
}

inline int8_t ram_read_byte_signed(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return (int8_t)ram.memory_read_hooks[address](ram, address);
  }
  return (int8_t)*ram_resolve_address(ram, address);
}

inline int8_t ram_read_byte_signed_direct(RAM& ram, uint32_t address) {
  return (int8_t)*ram_resolve_address(ram, address);
}

inline int16_t ram_read_half_word_signed(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return (int16_t)ram.memory_read_hooks[address](ram, address);
  }
  return (int16_t)ram_read_half_word_direct(ram, address);
}

inline int16_t ram_read_half_word_signed_direct(RAM& ram, uint32_t address) {
  return (int16_t)ram_read_half_word_direct(ram, address);
}

inline int32_t ram_read_word_signed(RAM& ram, uint32_t address) {
  if (ram_address_has_read_hook(ram, address)) {
    return (int32_t)ram.memory_read_hooks[address](ram, address);
  }
  return (int32_t)ram_read_word_direct(ram, address);
}

inline int32_t ram_read_word_signed_direct(RAM& ram, uint32_t address) {
  return (int32_t)ram_read_word_direct(ram, address);
}

inline void ram_write_byte(RAM& ram, uint32_t address, uint8_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
    return;
  }
  if (ram_address_has_write_hook(ram, address)) {
    ram.memory_write_hooks[address](ram, address, (uint32_t)value);
    return;
  }
  *ram_resolve_address(ram, address) = value;
}

// Direct write access without clear-on-write checks (for interrupt flags).
inline void ram_write_byte_direct(RAM& ram, uint32_t address, uint8_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
    return;
  }
  *ram_resolve_address(ram, address) = value;
}

inline void ram_write_half_word(RAM& ram, uint32_t address, uint16_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
    return;
  }
  if (ram_address_has_write_hook(ram, address)) {
    ram.memory_write_hooks[address](ram, address, (uint32_t)value);
    return;
  }
  *(uint16_t*)ram_resolve_address(ram, address) = value;
}

// Direct write access without clear-on-write checks (for interrupt flags).
inline void ram_write_half_word_direct(RAM& ram, uint32_t address, uint32_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
    return;
  }
  *(uint16_t*)ram_resolve_address(ram, address) = value;
}

inline void ram_write_word(RAM& ram, uint32_t address, uint32_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
    return;
  }
  if (ram_address_has_write_hook(ram, address)) {
    ram.memory_write_hooks[address](ram, address, value);
    return;
  }
  *(uint32_t*)ram_resolve_address(ram, address) = value;
}

// Direct write access without clear-on-write checks (for interrupt flags).
inline void ram_write_word_direct(RAM& ram, uint32_t address, uint32_t value) {
  if (ram.enable_rom_write_protection && address <= BIOS_END) {
    // std::cout << "Warning: Attempted to write to read-only memory, skipped write op" << std::endl;
  }
  *(uint32_t*)ram_resolve_address(ram, address) = value;
}
