#pragma once

#include "cpu.h"

struct Timer {
  std::array<uint32_t, 4> counters;
  std::array<uint32_t, 4> cycle_started;
};

void timer_init(CPU& cpu, Timer& timer);
void timer_tick(CPU& cpu, Timer& timer);
