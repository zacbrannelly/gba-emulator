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

void ram_load_rom(RAM& ram, std::string const& path) {
  load_binary(path, ram.game_pak_rom);
}

void ram_load_bios(RAM& ram, std::string const& path) {
  load_binary(path, ram.system_rom);
}
