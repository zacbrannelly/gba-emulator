#include "dma.h"

#define DMAxSAD(x) 0x40000B0 + (x * 12)
#define DMAxDAD(x) 0x40000B4 + (x * 12)
#define DMAxCNT_L(x) 0x40000B8 + (x * 12)
#define DMAxCNT_H(x) 0x40000BA + (x * 12)

#define DEFINE_DMA_CHANNEL(x) \
  static constexpr uint32_t DMA##x##SAD = DMAxSAD(x); \
  static constexpr uint32_t DMA##x##DAD = DMAxDAD(x); \
  static constexpr uint32_t DMA##x##CNT_L = DMAxCNT_L(x); \
  static constexpr uint32_t DMA##x##CNT_H = DMAxCNT_H(x);

DEFINE_DMA_CHANNEL(0)
DEFINE_DMA_CHANNEL(1)
DEFINE_DMA_CHANNEL(2)
DEFINE_DMA_CHANNEL(3)

static constexpr std::array<uint32_t, 4> DMA_SAD = {
  DMA0SAD,
  DMA1SAD,
  DMA2SAD,
  DMA3SAD
};

static constexpr std::array<uint32_t, 4> DMA_DAD = {
  DMA0DAD,
  DMA1DAD,
  DMA2DAD,
  DMA3DAD
};

static constexpr std::array<uint32_t, 4> DMA_CNT_L = {
  DMA0CNT_L,
  DMA1CNT_L,
  DMA2CNT_L,
  DMA3CNT_L
};

static constexpr std::array<uint32_t, 4> DMA_CNT_H = {
  DMA0CNT_H,
  DMA1CNT_H,
  DMA2CNT_H,
  DMA3CNT_H
};

static constexpr uint16_t DMA_CNT_L_ENABLE_FLAG = 1 << 15;

enum DMAChannel {
  DMA0 = 0,
  DMA1 = 1,
  DMA2 = 2,
  DMA3 = 3
};

enum DMADestinationAddressControl {
  DADIncrement = 0,
  DADDecrement = 1,
  DADFixed = 2,
  DADReload = 3
};

enum DMASourceAddressControl {
  SADIncrement = 0,
  SADDecrement = 1,
  SADFixed = 2,
  SADProhibited = 3
};

enum DMATransferType {
  TransferTypeHalfWord = 0,
  TransferTypeWord = 1
};

enum DMAStartMode {
  StartModeImmediate = 0,
  StartModeVBlank = 1,
  StartModeHBlank = 2,
  StartModeSpecial = 3
};

void dma_transfer(CPU& cpu, uint32_t source_addr, uint32_t dest_addr, DMATransferType transfer_type) {
  if (transfer_type == TransferTypeHalfWord) {
    uint16_t data = ram_read_half_word(cpu.ram, source_addr);
    ram_write_half_word(cpu.ram, dest_addr, data);
  } else {
    uint32_t data = ram_read_word(cpu.ram, source_addr);
    ram_write_word(cpu.ram, dest_addr, data);
  }
}

bool dma_process_channel(CPU& cpu, uint8_t channel) {
  uint32_t source_addr = ram_read_word(cpu.ram, DMA_SAD[channel]);
  uint32_t dest_addr = ram_read_word(cpu.ram, DMA_DAD[channel]);
  uint32_t word_count = ram_read_half_word(cpu.ram, DMA_CNT_L[channel]);
  uint32_t control = ram_read_half_word(cpu.ram, DMA_CNT_H[channel]);

  auto const dest_control = (DMADestinationAddressControl)((control >> 5) & 0x3);
  auto const source_control = (DMASourceAddressControl)((control >> 7) & 0x3);

  bool const is_repeat = (control >> 9) & 0x1;
  auto const transfer_type = (DMATransferType)((control >> 10) & 0x1);
  auto const start_mode = (DMAStartMode)((control >> 11) & 0x3);
  bool const irq_enable = (control >> 13) & 0x1;
  bool const enable = (control >> 15) & 0x1;

  if (!enable) {
    return false;
  }

  if (start_mode == StartModeVBlank) {
    uint16_t display_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);
    if ((display_status & 0x1) == 0) {
      return false;
    }
  } else if (start_mode == StartModeHBlank) {
    uint16_t display_status = ram_read_half_word(cpu.ram, REG_LCD_STATUS);
    if ((display_status & 0x2) == 0) {
      return false;
    }
  } else if (start_mode == StartModeSpecial) {
    // TODO: Implement special start mode when handling audio.
    return false;
  }

  uint32_t transfer_size = transfer_type == TransferTypeHalfWord ? 2 : 4;

  for (int i = 0; i < word_count; i++) {
    dma_transfer(cpu, source_addr, dest_addr, transfer_type);

    switch (dest_control) {
      case DADIncrement:
        dest_addr += transfer_size;
        break;
      case DADDecrement:
        dest_addr -= transfer_size;
        break;
      case DADFixed:
        break;
      case DADReload:
        dest_addr += transfer_size;
        break;
    }

    switch (source_control) {
      case SADIncrement:
        source_addr += transfer_size;
        break;
      case SADDecrement:
        source_addr -= transfer_size;
        break;
      case SADFixed:
        break;
      case SADProhibited:
        throw std::runtime_error("Prohibited source control mode.");
    }

    // Update the word count.
    ram_write_half_word(cpu.ram, DMA_CNT_L[channel], word_count - i - 1);
  }

  if (is_repeat) {
    // Reset word counter.
    ram_write_word(cpu.ram, DMA_CNT_L[channel], word_count);
  } else {
    // Disable the DMA channel.
    ram_write_half_word(cpu.ram, DMA_CNT_H[channel], control & ~DMA_CNT_L_ENABLE_FLAG);
  }

  // Generate an interrupt if enabled.
  if (irq_enable) {
    uint32_t interrupt_flags = ram_read_word(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS);
    interrupt_flags |= (1 << (8 + channel));
    ram_write_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
  }

  return true;
}

void dma_cycle(CPU& cpu) {
  for (uint8_t channel = 0; channel < 4; channel++) {
    // Process one channel per cycle.
    if (dma_process_channel(cpu, channel)) break;
  }
}
