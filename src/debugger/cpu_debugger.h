#pragma once

#include <queue>
#include <list>
#include <mutex>
#include "../cpu.h"

enum DebuggerMode {
  NORMAL,
  DEBUG
};

enum DebuggerCommand {
  CONTINUE,
  STEP,
  BREAK,
  NEXT_FRAME,
  RESET,
};

struct CPUState {
  uint32_t pc;
  uint32_t registers[16];
  uint32_t cpsr;
  uint32_t instruction;
  uint32_t irq_flags;
  uint32_t irq_enabled;
  bool irq_master_enabled;
};

struct DebuggerState {
  uint32_t breakpoint_address;
  uint32_t step_size = 1;
  DebuggerMode mode;
  std::queue<DebuggerCommand> command_queue;

  bool enable_record_state = false;
  bool ignore_bios_calls = true;
  int max_history_size = 1000;
  int history_page_size = 100000;
  int history_page = 0;
  std::list<CPUState> cpu_history;
  std::vector<std::list<CPUState>::iterator> cpu_history_pages;
  std::mutex cpu_history_mutex;
};

void cpu_record_state(CPU& cpu, DebuggerState& debugger_state);

void cpu_history_window(CPU& cpu, DebuggerState& debugger_state);
void cpu_debugger_window(CPU& cpu, DebuggerState& debugger_state);
