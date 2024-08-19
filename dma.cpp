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

#define DEFINE_DMA_CHANNEL_OFFSETS(x) \
  static constexpr uint32_t DMA_OFFSET##x##SAD = DMAxSAD(x) - 0x4000000; \
  static constexpr uint32_t DMA_OFFSET##x##DAD = DMAxDAD(x) - 0x4000000; \
  static constexpr uint32_t DMA_OFFSET##x##CNT_L = DMAxCNT_L(x) - 0x4000000; \
  static constexpr uint32_t DMA_OFFSET##x##CNT_H = DMAxCNT_H(x) - 0x4000000;

DEFINE_DMA_CHANNEL(0)
DEFINE_DMA_CHANNEL(1)
DEFINE_DMA_CHANNEL(2)
DEFINE_DMA_CHANNEL(3)

DEFINE_DMA_CHANNEL_OFFSETS(0)
DEFINE_DMA_CHANNEL_OFFSETS(1)
DEFINE_DMA_CHANNEL_OFFSETS(2)
DEFINE_DMA_CHANNEL_OFFSETS(3)

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

static constexpr uint32_t DMA_CNT_H[4] = {
  DMA0CNT_H,
  DMA1CNT_H,
  DMA2CNT_H,
  DMA3CNT_H
};

static constexpr uint32_t DMA_OFFSET_SAD[4] = {
  DMA_OFFSET0SAD,
  DMA_OFFSET1SAD,
  DMA_OFFSET2SAD,
  DMA_OFFSET3SAD
};

static constexpr uint32_t DMA_OFFSET_DAD[4] = {
  DMA_OFFSET0DAD,
  DMA_OFFSET1DAD,
  DMA_OFFSET2DAD,
  DMA_OFFSET3DAD
};

static constexpr uint32_t DMA_OFFSET_CNT_L[4] = {
  DMA_OFFSET0CNT_L,
  DMA_OFFSET1CNT_L,
  DMA_OFFSET2CNT_L,
  DMA_OFFSET3CNT_L
};

static constexpr uint32_t DMA_OFFSET_CNT_H[4] = {
  DMA_OFFSET0CNT_H,
  DMA_OFFSET1CNT_H,
  DMA_OFFSET2CNT_H,
  DMA_OFFSET3CNT_H
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

struct DMAControl {
  uint8_t reserved : 5;
  DMADestinationAddressControl destination_address_control : 2;
  DMASourceAddressControl source_address_control : 2;
  bool is_repeat : 1;
  bool is_word_transfer : 1;
  bool is_game_pak_drq : 1;
  DMAStartMode start_mode : 2;
  bool irq_enable : 1;
  bool enable : 1;
};

bool dma_process_channel(CPU& cpu, uint8_t channel) {
  uint32_t source_addr = *(uint32_t*)&cpu.ram.io_registers[DMA_OFFSET_SAD[channel]];
  uint32_t dest_addr = *(uint32_t*)&cpu.ram.io_registers[DMA_OFFSET_DAD[channel]];
  uint16_t word_count = *(uint16_t*)&cpu.ram.io_registers[DMA_OFFSET_CNT_L[channel]];
  uint16_t control = *(uint16_t*)&cpu.ram.io_registers[DMA_OFFSET_CNT_H[channel]];

  DMAControl dma_control = *(DMAControl*)&control;
  auto const dest_control = dma_control.destination_address_control;
  auto const source_control = dma_control.source_address_control;
  bool const is_repeat = dma_control.is_repeat;
  auto const transfer_type = dma_control.is_word_transfer ? TransferTypeWord : TransferTypeHalfWord;
  auto const start_mode = dma_control.start_mode;
  bool const irq_enable = dma_control.irq_enable;
  bool const enable = dma_control.enable;

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

  // If the word count is 0, transfer 0x10000 words for channel 3 and 0x4000 words for other channels.
  uint32_t final_word_count = (uint32_t)word_count;
  if (word_count == 0) {
    final_word_count = channel == 3 ? 0x10000 : 0x4000;
  }

  // std::cout << "DMA channel " << (int)channel << " transfer" << std::endl;
  // std::cout << "  Triggered by PC: 0x" << std::hex << cpu.get_register_value(PC) << std::endl;
  // std::cout << "  Source Address: 0x" << std::hex << source_addr << std::endl;
  // std::cout << "  Destination Address: 0x" << std::hex << dest_addr << std::endl;
  // std::cout << "  Word Count: " << std::dec << final_word_count << std::endl;
  // std::cout << "  Start Mode: " << (int)start_mode << std::endl;
  // std::cout << "  Repeat: " << is_repeat << std::endl;
  // std::cout << "  Control: 0x" << std::hex << control << std::endl;

  for (int i = 0; i < final_word_count; i++) {
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
    ram_write_half_word(cpu.ram, DMA_CNT_L[channel], final_word_count - i - 1);
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
    uint16_t interrupt_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
    interrupt_flags |= (1 << (8 + channel));
    ram_write_half_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
  }

  return true;
}

void dma_cycle(CPU& cpu) {
  for (uint8_t channel = 0; channel < 4; channel++) {
    // Process one channel per cycle.
    if (dma_process_channel(cpu, channel)) break;
  }
}
