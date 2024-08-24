#include "cpu_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

#include <sstream>

void cpu_record_state(CPU& cpu, DebuggerState& debugger_state) {
  if (!debugger_state.enable_record_state) return;
  if (debugger_state.ignore_bios_calls && cpu.get_register_value(PC) < 0x8000000) return;

  CPUState state;
  state.pc = cpu.get_register_value(PC);
  for (int i = 0; i < 16; i++) {
    state.registers[i] = cpu.get_register_value(i);
  }
  state.cpsr = cpu.cpsr;
  if (cpu.cpsr & CPSR_THUMB_STATE) {
    state.instruction = ram_read_half_word(cpu.ram, cpu.get_register_value(PC));
  } else {
    state.instruction = ram_read_word(cpu.ram, cpu.get_register_value(PC));
  }

  state.irq_enabled = ram_read_word_from_io_registers_fast<REG_INTERRUPT_ENABLE>(cpu.ram);
  state.irq_flags = ram_read_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
  state.irq_master_enabled = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_MASTER_ENABLE>(cpu.ram) & 0x1;

  debugger_state.cpu_history_mutex.lock();

  debugger_state.cpu_history.push_back(state);

  int history_size = debugger_state.cpu_history.size();
  if (history_size > debugger_state.max_history_size) {
    debugger_state.cpu_history.erase(debugger_state.cpu_history.begin());
  }

  if (history_size % debugger_state.history_page_size == 0) {
    debugger_state.cpu_history_pages.push_back(std::prev(debugger_state.cpu_history.end()));
  }

  debugger_state.cpu_history_mutex.unlock();
}

static int selected_history_index = -1;
static CPUState selected_cpu_state;

void cpu_history_window(CPU& cpu, DebuggerState& debugger_state) {
  if (ImGui::Begin("CPU History")) {
    ImGui::Checkbox("Record State", &debugger_state.enable_record_state);
    ImGui::Checkbox("Ignore BIOS Calls", &debugger_state.ignore_bios_calls);
    ImGui::InputInt("Max History Size", &debugger_state.max_history_size);
    
    if (ImGui::Button("Clear History")) {
      debugger_state.cpu_history_mutex.lock();

      debugger_state.cpu_history.clear();
      debugger_state.cpu_history_pages.clear();
      debugger_state.history_page = 0;
      selected_history_index = -1;

      debugger_state.cpu_history_mutex.unlock();
    }

    ImGui::BeginListBox("CPU History", ImVec2(300, 400));

    if (debugger_state.mode == DEBUG && debugger_state.cpu_history_mutex.try_lock()) {
      int i = 0;
      CPUState selected_state;
      auto begin = debugger_state.history_page > 0 
        ? debugger_state.cpu_history_pages[debugger_state.history_page - 1] 
        : debugger_state.cpu_history.begin();
      auto end = debugger_state.cpu_history_pages.size() > 0 && debugger_state.history_page < debugger_state.cpu_history_pages.size() - 1 
        ? debugger_state.cpu_history_pages[debugger_state.history_page + 1] 
        : debugger_state.cpu_history.end();
      for (auto it = begin; it != end; ++it) {
        // Show PC for each state as a selectable button.
        std::stringstream ss;
        ss << "0x" << std::hex << it->pc << " ##" << i;

        if (ImGui::Selectable(ss.str().c_str(), selected_history_index == i)) {
          selected_history_index = i;
          selected_cpu_state = *it;
        }
        i++;
      }
      debugger_state.cpu_history_mutex.unlock();
    }

    ImGui::EndListBox();

    if (ImGui::Button("Previous Page")) {
      if (debugger_state.history_page > 0) {
        debugger_state.history_page--;
      }
    }

    ImGui::SameLine();

    if (ImGui::Button("Next Page")) {
      if (debugger_state.history_page < debugger_state.cpu_history_pages.size() - 1) {
        debugger_state.history_page++;
      }
    }

    ImGui::SameLine();

    std::stringstream ss;
    ss << "Page " << debugger_state.history_page + 1 << " of " << debugger_state.cpu_history_pages.size();
    ImGui::Text("%s", ss.str().c_str());

    if (selected_history_index >= 0) {
      ImGui::Text("PC: 0x%08X", selected_cpu_state.pc);
      for (int i = 0; i < 16; i++) {
        ImGui::Text("R%d: 0x%08X", i, selected_cpu_state.registers[i]);
      }
      ImGui::Text("CPSR: 0x%08X", selected_cpu_state.cpsr);
      if (cpu.cpsr & CPSR_THUMB_STATE) {
        ImGui::Text("Instruction: 0x%04X", selected_cpu_state.instruction);
      } else {
        ImGui::Text("Instruction: 0x%08X", selected_cpu_state.instruction);
      }

      ImGui::Text("IRQ Enabled: 0x%08X", selected_cpu_state.irq_enabled);
      ImGui::Text("IRQ Flags: 0x%08X", selected_cpu_state.irq_flags);
      ImGui::Text("IRQ Master Enabled: %s", selected_cpu_state.irq_master_enabled ? "true" : "false");
    }
  }
  ImGui::End();
}

void cpu_debugger_window(CPU& cpu, DebuggerState& debugger_state) {
  if (ImGui::Begin("CPU Debugger")) {
    // Break / Continue button.
    if (debugger_state.mode == NORMAL) {
      if (ImGui::Button("Break")) {
        debugger_state.command_queue.push(BREAK);
      }
    } else {
      if (ImGui::Button("Continue")) {
        debugger_state.command_queue.push(CONTINUE);
      }
    }

    // Step button.
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
      debugger_state.command_queue.push(STEP);
    }

    // Next Frame button.
    ImGui::SameLine();
    if (ImGui::Button("Next Frame")) {
      debugger_state.command_queue.push(NEXT_FRAME);
    }

    // Reset button.
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
      debugger_state.command_queue.push(RESET);
    }

    // Breakpoint.
    ImGui::InputScalar("Breakpoint", ImGuiDataType_U32, &debugger_state.breakpoint_address, 0, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);

    // Step size.
    ImGui::InputScalar("Step Size", ImGuiDataType_U32, &debugger_state.step_size, 0, 0, "%d", ImGuiInputTextFlags_CharsDecimal);

    ImGui::Text("PC: 0x%08X", cpu.get_register_value(PC));
    for (int i = 0; i < 16; i++) {
      ImGui::Text("R%d: 0x%08X", i, cpu.get_register_value(i));
    }
    ImGui::Text("CPSR: 0x%08X", cpu.cpsr);

    if (cpu.cpsr & CPSR_THUMB_STATE) {
      ImGui::Text("Instruction: 0x%04X", ram_read_half_word(cpu.ram, cpu.get_register_value(PC)));
    } else {
      ImGui::Text("Instruction: 0x%08X", ram_read_word(cpu.ram, cpu.get_register_value(PC)));
    }

    uint16_t reg_interrupt_enable = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_ENABLE>(cpu.ram);
    uint16_t reg_interrupt_request_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
    uint16_t reg_interrupt_master_enable = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_MASTER_ENABLE>(cpu.ram);
    ImGui::Text("Interrupt Enable: 0x%04X", reg_interrupt_enable);
    ImGui::Text("Interrupt Request Flags: 0x%04X", reg_interrupt_request_flags);
    ImGui::Text("Interrupt Master Enable: 0x%04X", reg_interrupt_master_enable);

    ImGui::Text("Cycle Count: %llu", cpu.cycle_count);
  }
  ImGui::End();
}
