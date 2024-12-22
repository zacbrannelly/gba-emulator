#include "ram.h"
#include <fstream>
#include <cstring>

void load_binary(std::string const& path, uint8_t* dest) {
  std::ifstream bin_in(path, std::ios::binary);
  if (!bin_in.is_open()) {
    throw std::runtime_error("Error: Could not open file " + path);
  }

  // Read binary size.
  bin_in.seekg(0, std::ios::end);
  size_t bin_size = bin_in.tellg();

  // Copy binary data to memory.
  uint8_t* bin_data = new uint8_t[bin_size];
  bin_in.seekg(0, std::ios::beg);
  bin_in.read((char*)bin_data, bin_size);

  // Close binary file.
  bin_in.close();

  // Copy binary data to memory.
  memcpy(dest, bin_data, bin_size);

  delete [] bin_data;
}

void ram_init(RAM& ram) {
  // Clear-on-write when writing to the interrupt request flags.
  ram_register_write_hook(ram, REG_INTERRUPT_REQUEST_FLAGS, [](RAM& ram, uint32_t address, uint32_t value) {
    *(uint32_t*)ram_resolve_address(ram, address) &= ~value;
  });

  // Make sure the key status register is read-only.
  ram_register_write_hook(ram, REG_KEY_STATUS, [](RAM&, uint32_t, uint32_t) {
    // Do nothing.
  });

  // For EEPROM, if the user does a read from 0xd000000, return 0x1 to indicate that the write request is complete.
  // TODO: This needs to be more integrated with the EEPROM module.
  // TODO: Some games might not have an EEPROM, so this needs to be configurable.
  ram_register_read_hook(ram, 0xd000000, [](RAM& ram, uint32_t address) {
    return 0x1;
  });

  // Initialize the EEPROM with all bits set to 1 (to match MGBA behavior).
  memset(ram.eeprom, 0xFF, 0x2000);
}

void ram_soft_reset(RAM& ram) {
  // TODO: Put the size of each memory region in a constant.
  memset(ram.external_working_ram, 0, 0x40000);
  memset(ram.internal_working_ram, 0, 0x8000);
  memset(ram.io_registers, 0, 0x804);
  memset(ram.palette_ram, 0, 0x400);
  memset(ram.video_ram, 0, 0x18000);
  memset(ram.object_attribute_memory, 0, 0x400);

  // TODO: Reset EEPROM memory here, but make it configurable so you can keep save data.

  // Make sure REG_KEY_STATUS is set to all keys being released.
  ram_write_half_word_to_io_registers_fast<REG_KEY_STATUS>(ram, 0x3FF);

  // Supply a dummy value for the Flash ID, which is used by some games to detect the presence of a flash memory chip.
  ram_write_byte_direct(ram, GAME_PAK_SRAM_START, 0x62);
  ram_write_byte_direct(ram, GAME_PAK_SRAM_START + 1, 0x13);
}

void ram_load_rom(RAM& ram, std::string const& path) {
  if (ram.load_rom_into_bios) {
    ram_load_bios(ram, path);
    return;
  }
  load_binary(path, ram.game_pak_rom);
}

void ram_load_bios(RAM& ram, std::string const& path) {
  load_binary(path, ram.system_rom);
}

void ram_register_read_hook(RAM& ram, uint32_t address, std::function<uint32_t(RAM&, uint32_t)> const& hook) {
  ram.memory_read_hooks[address] = hook;
  ram.memory_read_hook_addresses.push_back(address);
}

void ram_register_write_hook(RAM& ram, uint32_t address, std::function<void(RAM&, uint32_t, uint32_t)> const& hook) {
  ram.memory_write_hooks[address] = hook;
  ram.memory_write_hook_addresses.push_back(address);
}
