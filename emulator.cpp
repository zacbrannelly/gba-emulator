#include <fstream>
#include <string>
#include <iostream>
#include <bitset>

#include "cpu.h"

int main(int argc, char* argv[]) {
  CPU cpu;
  cpu_init(cpu);
  ram_load_rom(cpu.ram, "pokemon_emerald.gba");

  for (int i = 0; i < 10; i++) {
    cpu_cycle(cpu);
  }

  return 0;
}
