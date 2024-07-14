#include <fstream>
#include <string>
#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>
#include <queue>

#include "debug.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "timer.h"

#include "3rdparty/zengine/ZEngine-Core/Misc/Factory.h"
#include "3rdparty/zengine/ZEngine-Core/Input/InputManager.h"
#include "3rdparty/zengine/ZEngine-Core/Physics/Time.h"
#include "3rdparty/zengine/ZEngine-Core/Display/Display.h"
#include "3rdparty/zengine/ZEngine-Core/GameLoop/GameLoop.h"
#include "3rdparty/zengine/ZEngine-Core/Rendering/Graphics.h"
#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/GUILibrary.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

enum DebuggerMode {
  NORMAL,
  DEBUG
};

enum DebuggerCommand {
  CONTINUE,
  STEP,
  BREAK,
  NEXT_FRAME,
};

struct DebuggerState {
  DebuggerMode mode;
  std::queue<DebuggerCommand> command_queue;
};

void cycle(CPU& cpu, Timer& timer) {
  // PC alignment check.
  if (cpu.cspr & CSPR_THUMB_STATE) {
    if (cpu.get_register_value(PC) % 2 != 0) {
      throw std::runtime_error("PC is not aligned to 2 bytes.");
    }
  } else {
    if (cpu.get_register_value(PC) % 4 != 0) {
      throw std::runtime_error("PC is not aligned to 4 bytes.");
    }
  }

  cpu_cycle(cpu);
  cpu_interrupt_cycle(cpu);

  // Cycle the GPU.
  gpu_cycle(cpu);

  // Cycle the DMA controller.
  dma_cycle(cpu);

  // Cycle the Timer controller.
  timer_tick(cpu, timer);

  cpu.cycle_count++;
}

void emulator_loop(CPU& cpu, Timer& timer, DebuggerState& debugger_state) {
  cpu_init(cpu);
  timer_init(cpu, timer);
  
  ram_load_bios(cpu.ram, "gba_bios.bin");
  ram_load_rom(cpu.ram, "pokemon_emerald.gba");

  // Start from the beginning of the ROM.
  cpu.set_register_value(PC, 0x0);

  // Set the SP to the end of the working RAM on-chip (matching VirtualBoy Advance).
  cpu.set_register_value(SP, WORKING_RAM_ON_CHIP_END - 0xFF);

  // Set the SP for each mode.
  // TODO: Clean this up and move it to a function (perhaps cpu_init?).
  uint32_t cspr_backup = cpu.cspr;
  cpu.cspr = (uint32_t)Supervisor;
  cpu.set_register_value(SP, 0x3007FE0);

  cpu.cspr = (uint32_t)IRQ;
  cpu.set_register_value(SP, 0x3007FA0);

  cpu.cspr = (uint32_t)System;
  cpu.set_register_value(SP, 0x3007F00);

  // Reset the CSPR to the original value.
  cpu.cspr = cspr_backup;

  while(!cpu.killSignal) {
    if (debugger_state.command_queue.size() > 0) {
      // Get the first command from the queue.
      auto command = debugger_state.command_queue.front();
      debugger_state.command_queue.pop();

      // Process the command.
      switch (command) {
        case CONTINUE:
          debugger_state.mode = NORMAL;
          break;
        case STEP:
          cycle(cpu, timer);
          debugger_state.mode = DEBUG;
          break;
        case BREAK:
          debugger_state.mode = DEBUG;
          break;
        case NEXT_FRAME:
          uint8_t scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          bool hit_vcount = false;
          while (scanline != 160) {
            cycle(cpu, timer);
            scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          }
          while (scanline == 160) {
            cycle(cpu, timer);
            scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          }
          break;
      }
    }

    if (debugger_state.mode == DEBUG) {
      // Wait for a command from the debugger.
      continue;
    }

    cycle(cpu, timer);
  }
}

static constexpr int VIEW_ID = 0;
static constexpr uint16_t COLOR_RED = 0x1F;

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

    ImGui::Text("PC: 0x%08X", cpu.get_register_value(PC));
    for (int i = 0; i < 16; i++) {
      ImGui::Text("R%d: 0x%08X", i, cpu.get_register_value(i));
    }
    ImGui::Text("CSPR: 0x%08X", cpu.cspr);

    if (cpu.cspr & CSPR_THUMB_STATE) {
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

    ImGui::Text("Cycle Count: %d", cpu.cycle_count);

    ImGui::End();
  }
}

void graphics_loop(CPU& cpu, DebuggerState& debugger_state) {
  Factory::Init();

  Display display("GBA Emulator", 1024, 700);
  if (!display.Init()) {
    throw std::runtime_error("Failed to initialize display.");
  }

  auto inputManager = InputManager::GetInstance();
  inputManager->Init(&display);

  auto graphics = Graphics::GetInstance();
  graphics->Init(&display);

  auto time = Time::GetInstance();
  time->Init();

  auto gui = GUILibrary::GetInstance();
  gui->Init(&display);

  std::vector<uint32_t> frameData(240 * 160, 0);
  for (int i = 0; i < 240 * 160; i++) {
    frameData[i] = 0xFFFFFFFF;
  }

  auto frameTexture = new Texture2D(240, 160, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE);
  frameTexture->Update(0, 0, 240, 160, frameData.data(), frameData.size() * sizeof(uint32_t), 240 * sizeof(uint32_t));

  std::function<void()> updateCallback = [&]() {};

  std::function<void()> renderCallback = [&]() {
    gui->NewFrame();

    if (ImGui::Begin("GBA Emulator")) {
      ImGui::Image(frameTexture->GetHandle(), ImVec2(240 * 2, 160 * 2));
      ImGui::End();
    }

    cpu_debugger_window(cpu, debugger_state);

    gui->EndFrame();

    graphics->Clear(VIEW_ID, 20, 20, 20, 255);
    graphics->Viewport(VIEW_ID, 0, 0, display.GetWidth(), display.GetHeight());
    graphics->Touch(VIEW_ID);

    // TODO: We need to render by scanline.
    // TODO: Figure out how to create a pixel buffer that we can easily write to.

    graphics->Render();
    inputManager->ClearMouseDelta();
  };

  GameLoop loop(&display, 1.0 / 60.0, updateCallback, renderCallback);
  loop.StartLoop();

  cpu.killSignal = true;

  display.Shutdown();
  gui->Shutdown();
  graphics->Shutdown();
  inputManager->Shutdown();
  time->Shutdown();
}

void start_cpu_loop(CPU& cpu, Timer& timer, DebuggerState& debugger_state) {
  try {
    emulator_loop(cpu, timer, debugger_state);
  } catch (std::exception& e) {
    debug_print_cpu_state(cpu);
    std::cout << e.what() << std::endl;
  }
}

int main(int argc, char* argv[]) {
  CPU cpu;
  Timer timer;
  DebuggerState debugger_state;

  // Start with the debugger set to break straight away.
  debugger_state.mode = DEBUG;

  // Run CPU in a separate thread.
  std::thread cpu_thread(start_cpu_loop, std::ref(cpu), std::ref(timer), std::ref(debugger_state));

  // Run graphics in the main thread.
  graphics_loop(cpu, debugger_state);

  cpu_thread.join();

  return 0;
}
