#include "eeprom.h"
#include "cpu.h"
#include "ram.h"

#define EEPROM_COMMAND_READ 0x3
#define EEPROM_COMMAND_WRITE 0x2

static int32_t eeprom_read_address = -1;

// Store max 128 bits (16 bytes) of data.
// Realistically we should only see max 68 bits of data.
static uint16_t eeprom_half_word_buffer[128];

void eeprom_dma_transfer(
  CPU& cpu,
  uint32_t source_addr,
  uint32_t dest_addr,
  uint32_t idx
) {
  if (dest_addr >= 0xd000000 && dest_addr < 0xdffffff) {
    // Read bit 0 from the source memory address.
    uint16_t data = ram_read_half_word(cpu.ram, source_addr) & 0x1;
    eeprom_half_word_buffer[idx] = data;
  } else if (eeprom_read_address >= 0) {
    // Write the data to RAM in 16-bit chunks, where the first bit is the data bit.
    uint64_t data = ((uint64_t*)cpu.ram.eeprom)[eeprom_read_address];

    // Skip first 4 bits.
    if (idx < 4) return;

    // Write the data to the buffer.
    uint16_t chunk = (data >> (63 - (idx - 4))) & 0x1;
    ram_write_half_word(cpu.ram, dest_addr, chunk);
  } else {
    // Invalid EEPROM address.
    throw std::runtime_error("Invalid EEPROM read address.");
  }
}

void eeprom_execute_command(CPU& cpu, uint16_t bit_count) {
  // Get command (first 2 bits of the buffer).
  // READ = 0b11, WRITE = 0b10
  uint8_t command = (eeprom_half_word_buffer[0] << 1) | eeprom_half_word_buffer[1];

  if (command == EEPROM_COMMAND_READ) {
    // Set the read address for the next DMA request.
    eeprom_read_address = 0;

    // TODO: This is a crude way to determine the address size.
    // TODO: Need a game database to determine the address size.
    int addr_size = bit_count > 9 ? 14 : 6;
    for (int i = 0; i < addr_size; i++) {
      eeprom_read_address = (eeprom_read_address << 1) | eeprom_half_word_buffer[i + 2];
    }
  } else if (command == EEPROM_COMMAND_WRITE) {
    uint16_t write_address = 0;
    uint64_t write_data = 0;

    // TODO: This is also a crude way to determine the address size.
    int addr_size = bit_count == 73 ? 6 : 14;
    for (int i = 0; i < 64; i++) {
      write_address = (write_data << 1) | eeprom_half_word_buffer[i + 2];
    }

    for (int i = 0; i < 64; i++) {
      write_data = (write_data << 1) | eeprom_half_word_buffer[i + addr_size + 2];
    }

    // Write 64 bits of data to the EEPROM.
    ((uint64_t*)cpu.ram.eeprom)[write_address] = write_data;
  } else {
    // Invalid command.
    throw std::runtime_error("Invalid EEPROM command.");
  }
}
