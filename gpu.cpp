#include "gpu.h"
#include "debug.h"

static constexpr uint16_t REG_LCD_STATUS_HBLANK_FLAG = 1 << 1;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_FLAG = 1 << 2;
static constexpr uint16_t REG_LCD_STATUS_VBLANK_INTERRUPT_ENABLE = 1 << 3;
static constexpr uint16_t REG_LCD_STATUS_HBLANK_INTERRUPT_ENABLE = 1 << 4;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_INTERRUPT_ENABLE = 1 << 5;

void gpu_complete_scanline(CPU& cpu) {
  uint8_t scanline = ram_read_byte(cpu.ram, REG_VERTICAL_COUNT);
  uint16_t lcd_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);

  // Begin VCount match
  uint8_t vertical_count_setting = lcd_status >> 8;
  if (scanline == vertical_count_setting) {
    lcd_status |= REG_LCD_STATUS_VCOUNT_MATCH_FLAG;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);

    // Request VCount interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_VCOUNT_MATCH_INTERRUPT_ENABLE) {
      uint16_t interrupt_flags = ram_read_half_word(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS);
      interrupt_flags |= REG_LCD_STATUS_VCOUNT_MATCH_FLAG;
      ram_write_half_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
    }
  } else if (lcd_status & REG_LCD_STATUS_VCOUNT_MATCH_FLAG) {
    // End VCount match
    lcd_status &= ~REG_LCD_STATUS_VCOUNT_MATCH_FLAG;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);
  }

  // Begin VBlank
  if (scanline == 160) {
    std::cout << "VBlank" << std::endl;
    lcd_status |= 0x1;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);

    // Request VBlank interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_VBLANK_INTERRUPT_ENABLE) {
      std::cout << "VBlank interrupt" << std::endl;
      uint16_t interrupt_flags = ram_read_half_word(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS);
      interrupt_flags |= 0x1;
      ram_write_half_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
    }
  }

  // End VBlank
  if (scanline == 226) {
    uint16_t lcd_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);
    lcd_status &= ~0x1;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);
  }

  // Increment scanline
  scanline++;
  if (scanline == 228) {
    scanline = 0;
  }
  ram_write_byte(cpu.ram, REG_VERTICAL_COUNT, scanline);

  if (scanline == 0) {
    // print_screen_state(cpu);
  }
}

void gpu_cycle(CPU& cpu) {
  if (cpu.cycle_count % 1232 == 0) {
    gpu_complete_scanline(cpu);
  }

  // Calculate how many cycles we are into the current scanline
  uint32_t cycles_into_scanline = 1232 - cpu.cycle_count % 1232;
  if (cycles_into_scanline == 1232 - 272) {
    // Begin HBlank
    uint16_t lcd_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);
    lcd_status |= 1 << 1;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);

    // Request HBlank interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_HBLANK_INTERRUPT_ENABLE) {
      std::cout << "HBlank interrupt" << std::endl;
      uint16_t interrupt_flags = ram_read_half_word(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS);
      interrupt_flags |= REG_LCD_STATUS_HBLANK_FLAG;
      ram_write_half_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
    }
  } else if (cycles_into_scanline == 0) {
    // End HBlank
    uint16_t lcd_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);
    lcd_status &= ~REG_LCD_STATUS_HBLANK_FLAG;
    ram_write_half_word(cpu.ram, REG_LCD_STATUS, lcd_status);
  }
}
