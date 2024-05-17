#include <fstream>
#include <string>
#include <iostream>
#include <bitset>

#include "cpu.h"

void load_rom(CPU& cpu, const std::string& filename) {
  std::ifstream bin_in(filename, std::ios::binary);
  if (!bin_in.is_open()) {
    throw std::runtime_error("Error: Could not open file " + filename);
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
  memcpy(cpu.memory, bin_data, bin_size);

  delete [] bin_data;
}

int main(int argc, char* argv[]) {
  CPU cpu;
  cpu_init(cpu);
  load_rom(cpu, "pokemon_emerald.gba");

  for (int i = 0; i < 10; i++) {
    cpu_cycle(cpu);
  }

  return 0;
}
