#include "timer.h"

#define TMXCNT_L(x) 0x4000100 + (x * 4)
#define TMXCNT_H(x) 0x4000102 + (x * 4)

#define DEFINE_TIMER(x) \
  static constexpr uint32_t TM##x##CNT_L = TMXCNT_L(x); \
  static constexpr uint32_t TM##x##CNT_H = TMXCNT_H(x);

DEFINE_TIMER(0)
DEFINE_TIMER(1)
DEFINE_TIMER(2)
DEFINE_TIMER(3)

// Counter / Reload Register Address
static constexpr std::array<uint32_t, 4> TM_CNT_L = {
  TM0CNT_L,
  TM1CNT_L,
  TM2CNT_L,
  TM3CNT_L
};

// Control Register Address
static constexpr std::array<uint32_t, 4> TM_CNT_H = {
  TM0CNT_H,
  TM1CNT_H,
  TM2CNT_H,
  TM3CNT_H
};

enum TimerPrescalerSelection {
  Prescaler1 = 0,
  Prescaler64 = 1,
  Prescaler256 = 2,
  Prescaler1024 = 3
};

static constexpr std::array<uint32_t, 4> TM_PRESCALER_VALUES = {
  1,
  64,
  256,
  1024
};

static constexpr uint16_t TM_CNT_H_ENABLE_FLAG = 1 << 7;

void timer_init(CPU& cpu, Timer& timer) {
  for (int i = 0; i < 4; ++i) {
    // Initialize the counters.
    timer.counters[i] = 0;

    ram_register_read_hook(cpu.ram, TM_CNT_L[i], [i, &timer](RAM& ram, uint32_t address) {
      // RAM Read returns the counter value.
      return timer.counters[i];
    });

    // Make sure the counter is reset with the <reload> value if the timer is enabled.
    ram_register_write_hook(cpu.ram, TM_CNT_H[i], [i, &timer](RAM& ram, uint32_t address, uint32_t value) {
      uint16_t prev_value = ram_read_half_word_direct(ram, TM_CNT_H[i]);
      if (
        (prev_value & TM_CNT_H_ENABLE_FLAG) == 0 &&
        (value & TM_CNT_H_ENABLE_FLAG) > 0
      ) {
        // If the timer is disabled, set the counter to the <reload> value.
        timer.counters[i] = ram_read_half_word_direct(ram, TM_CNT_L[i]);
      }
      ram_write_half_word_direct(ram, TM_CNT_H[i], (uint16_t)value);
    });
  }
}

void timer_tick(CPU& cpu, Timer& timer) {
  // Reset the overflow flags.
  for (int i = 0; i < 4; ++i) {
    timer.overflow_flags[i] = false;
  }

  for (int i = 0; i < 4; ++i) {
    uint32_t control = ram_read_half_word_direct(cpu.ram, TM_CNT_H[i]);
    bool const enabled = control & TM_CNT_H_ENABLE_FLAG;
    if (!enabled) continue;

    auto const scale = (TimerPrescalerSelection)(control & 0x3);
    auto const interval = TM_PRESCALER_VALUES[scale];
    bool const count_up_on_prev_overflow = control & (1 << 2);

    if (count_up_on_prev_overflow && i > 0) {
      // If the previous timer overflowed, increment the current timer.
      if (timer.overflow_flags[i - 1]) {
        timer.counters[i]++;
      }
    } else if (cpu.cycle_count % interval == 0) {
      timer.counters[i]++;
    }

    // Detect overflow (16-bit counter)
    if (timer.counters[i] > 0xFFFF) {
      timer.overflow_flags[i] = true;

      // Load the <reload> value into the counter.
      timer.counters[i] = ram_read_half_word_direct(cpu.ram, TM_CNT_L[i]);

      // Trigger IRQ if enabled on overflow.
      bool const irq_enabled = control & (1 << 6);
      if (irq_enabled) {
        uint16_t interrupt_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
        interrupt_flags |= (1 << (3 + i));
        ram_write_half_word_direct(cpu.ram, REG_INTERRUPT_REQUEST_FLAGS, interrupt_flags);
      }
    }
  }
}
