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
