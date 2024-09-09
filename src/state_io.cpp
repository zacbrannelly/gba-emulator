#include "state_io.h"
#include "ram.h"

#include <cstring>
#include <fstream>

struct SaveState {
  // CPU
  uint64_t cycle_count = 0;
  uint32_t registers[16];
  uint32_t cpsr;
  uint32_t scpsr_registers[5] = {0, 0, 0, 0, 0};
  uint32_t banked_registers[5][7] = {
    {0, 0, 0, 0, 0, 0, 0}, // FIQ
    {0, 0, 0, 0, 0, 0, 0}, // IRQ
    {0, 0, 0, 0, 0, 0, 0}, // Supervisor
    {0, 0, 0, 0, 0, 0, 0}, // Abort
    {0, 0, 0, 0, 0, 0, 0}  // Undefined
  };

  // RAM
  uint8_t external_working_ram[0x40000];
  uint8_t internal_working_ram[0x8000];
  uint8_t io_registers[0x804];
  uint8_t palette_ram[0x400];
  uint8_t vram[0x18000];
  uint8_t oam[0x400];
  uint8_t game_pak_sram[0x10000];
};

void save_state(CPU& cpu, std::string const& state_file_path) {
  SaveState state;
  state.cycle_count = cpu.cycle_count;
  state.cpsr = cpu.cpsr;
  memcpy(state.registers, cpu.registers, sizeof(state.registers));
  memcpy(state.banked_registers, cpu.banked_registers, sizeof(state.banked_registers));

  state.scpsr_registers[0] = cpu.mode_to_scpsr[FIQ];
  state.scpsr_registers[1] = cpu.mode_to_scpsr[IRQ];
  state.scpsr_registers[2] = cpu.mode_to_scpsr[Supervisor];
  state.scpsr_registers[3] = cpu.mode_to_scpsr[Abort];
  state.scpsr_registers[4] = cpu.mode_to_scpsr[Undefined];

  memcpy(state.external_working_ram, cpu.ram.external_working_ram, sizeof(state.external_working_ram));
  memcpy(state.internal_working_ram, cpu.ram.internal_working_ram, sizeof(state.internal_working_ram));
  memcpy(state.io_registers, cpu.ram.io_registers, sizeof(state.io_registers));
  memcpy(state.palette_ram, cpu.ram.palette_ram, sizeof(state.palette_ram));
  memcpy(state.vram, cpu.ram.video_ram, sizeof(state.vram));
  memcpy(state.oam, cpu.ram.object_attribute_memory, sizeof(state.oam));
  memcpy(state.game_pak_sram, cpu.ram.game_pak_sram, sizeof(state.game_pak_sram));

  std::ofstream file(state_file_path, std::ios::binary);
  file.write(reinterpret_cast<char*>(&state), sizeof(state));
  file.close();
}

void load_state(CPU& cpu, std::string const& state_file_path) {
  SaveState state;

  std::ifstream file(state_file_path, std::ios::binary);
  file.read(reinterpret_cast<char*>(&state), sizeof(state));
  file.close();

  cpu.cycle_count = state.cycle_count;
  cpu.cpsr = state.cpsr;
  memcpy(cpu.registers, state.registers, sizeof(state.registers));
  memcpy(cpu.banked_registers, state.banked_registers, sizeof(state.banked_registers));

  cpu.mode_to_scpsr[FIQ] = state.scpsr_registers[0];
  cpu.mode_to_scpsr[IRQ] = state.scpsr_registers[1];
  cpu.mode_to_scpsr[Supervisor] = state.scpsr_registers[2];
  cpu.mode_to_scpsr[Abort] = state.scpsr_registers[3];
  cpu.mode_to_scpsr[Undefined] = state.scpsr_registers[4];

  memcpy(cpu.ram.external_working_ram, state.external_working_ram, sizeof(state.external_working_ram));
  memcpy(cpu.ram.internal_working_ram, state.internal_working_ram, sizeof(state.internal_working_ram));
  memcpy(cpu.ram.io_registers, state.io_registers, sizeof(state.io_registers));
  memcpy(cpu.ram.palette_ram, state.palette_ram, sizeof(state.palette_ram));
  memcpy(cpu.ram.video_ram, state.vram, sizeof(state.vram));
  memcpy(cpu.ram.object_attribute_memory, state.oam, sizeof(state.oam));
  memcpy(cpu.ram.game_pak_sram, state.game_pak_sram, sizeof(state.game_pak_sram));
}
