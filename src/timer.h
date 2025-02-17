#pragma once

#include "cpu.h"

struct Timer {
  uint32_t counters[4] = {0, 0, 0, 0};
  bool overflow_flags[4] = {false, false, false, false};
};

void timer_init(CPU& cpu, Timer& timer);
void timer_tick(CPU& cpu, Timer& timer);
