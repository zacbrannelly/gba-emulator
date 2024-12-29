#pragma once

#include <stdint.h>

struct CPU;

enum FlashMode {
  READ,
  ID_MODE,
  ERASE_MODE,
  WRITE_MODE,
  SELECT_BANK_MODE,
};

struct Flash {
  uint8_t command_buffer[3];
  uint8_t command_buffer_index;
  uint8_t bank;
  FlashMode mode;
};

void flash_init(CPU& cpu);
void flash_write_byte(CPU& cpu, uint32_t address, uint8_t value);
uint8_t flash_read_byte(CPU& cpu, uint32_t address);
