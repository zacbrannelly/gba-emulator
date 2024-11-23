#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

struct CPU;

void eeprom_dma_transfer(
  CPU& cpu,
  uint32_t source_addr,
  uint32_t dest_addr,
  uint32_t idx
);

void eeprom_execute_command(CPU& cpu, uint16_t bit_count);

#endif // EEPROM_H_