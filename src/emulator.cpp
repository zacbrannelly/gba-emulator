#include <fstream>
#include <string>
#include <iostream>
#include <bitset>
#include <chrono>
#include <thread>

#include "debug.h"
#include "cpu.h"
#include "dma.h"
#include "gpu.h"
#include "timer.h"
#include "input.h"
#include "debugger/palette_debugger.h"
#include "debugger/sprite_debugger.h"
#include "debugger/ram_debugger.h"
#include "debugger/special_effects_debugger.h"
#include "debugger/window_debugger.h"
#include "debugger/bg_debugger.h"
#include "debugger/cpu_debugger.h"
#include "debugger/state_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Misc/Factory.h"
#include "3rdparty/zengine/ZEngine-Core/Input/InputManager.h"
#include "3rdparty/zengine/ZEngine-Core/Physics/Time.h"
#include "3rdparty/zengine/ZEngine-Core/Display/Display.h"
#include "3rdparty/zengine/ZEngine-Core/GameLoop/GameLoop.h"
#include "3rdparty/zengine/ZEngine-Core/Rendering/Graphics.h"
#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/GUILibrary.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

void cycle(CPU& cpu, GPU& gpu, Timer& timer, DebuggerState& debugger_state) {
  // PC alignment check.
  if (cpu.cpsr & CPSR_THUMB_STATE) {
    if (cpu.get_register_value(PC) % 2 != 0) {
      throw std::runtime_error("PC is not aligned to 2 bytes.");
    }
  } else {
    if (cpu.get_register_value(PC) % 4 != 0) {
      throw std::runtime_error("PC is not aligned to 4 bytes.");
    }
  }

  // Record the current state of the CPU for debugging purposes.
  cpu_record_state(cpu, debugger_state);

  cpu_cycle(cpu);
  cpu_interrupt_cycle(cpu);

  // Cycle the GPU.
  gpu_cycle(cpu, gpu);

  // Cycle the DMA controller.
  dma_cycle(cpu);

  // Cycle the Timer controller.
  timer_tick(cpu, timer);

  cpu.cycle_count++;
}

void reset_cpu(CPU& cpu, GPU& gpu, Timer& timer) {
  // TODO: Move this into the cpu module.
  for (int i = 0; i < 16; i++) {
    cpu.set_register_value(i, 0);
  }
  cpu.cpsr = (uint32_t)System | CPSR_FIQ_DISABLE;
  cpu.cycle_count = 0;

  ram_soft_reset(cpu.ram);
}

void emulator_loop(
  CPU& cpu,
  GPU& gpu,
  Timer& timer,
  DebuggerState& debugger_state
) {
  cpu_init(cpu);
  gpu_init(cpu, gpu);
  timer_init(cpu, timer);
  
  ram_load_bios(cpu.ram, "gba_bios.bin");
  
  // TODO: Make this configurable in the debugger UI.
  // ram_load_rom(cpu.ram, "AGB_CHECKER_TCHK30.gba");
  ram_load_rom(cpu.ram, "pokemon_emerald.gba");

  // Start from the beginning of the ROM.
  cpu.set_register_value(PC, 0x0);

  // Set the key status to all keys being released (0 = pressed, 1 = released).
  ram_write_half_word_to_io_registers_fast<REG_KEY_STATUS>(cpu.ram, 0x3FF);

  // Supply a dummy value for the Flash ID, which is used by some games to detect the presence of a flash memory chip.
  ram_write_byte_direct(cpu.ram, GAME_PAK_SRAM_START, 0x62);
  ram_write_byte_direct(cpu.ram, GAME_PAK_SRAM_START + 1, 0x13);

  while(!cpu.killSignal) {
    if (debugger_state.command_queue.size() > 0) {
      // Get the first command from the queue.
      auto command = debugger_state.command_queue.front();
      debugger_state.command_queue.pop();

      // Process the command.
      switch (command) {
        case CONTINUE:
          cycle(cpu, gpu, timer, debugger_state);
          debugger_state.mode = NORMAL;
          break;
        case STEP:
          for (int i = 0; i < debugger_state.step_size; i++) {
            cycle(cpu, gpu, timer, debugger_state);
          }
          debugger_state.mode = DEBUG;
          break;
        case BREAK:
          debugger_state.mode = DEBUG;
          break;
        case RESET:
          reset_cpu(cpu, gpu, timer);
          break;
        case NEXT_FRAME:
          uint8_t scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          bool hit_vcount = false;
          while (scanline != 160) {
            cycle(cpu, gpu, timer, debugger_state);
            scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          }
          while (scanline == 160) {
            cycle(cpu, gpu, timer, debugger_state);
            scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
          }
          break;
      }
    }

    if (cpu.get_register_value(PC) == debugger_state.breakpoint_address) {
      debugger_state.mode = DEBUG;
    }

    if (debugger_state.mode == DEBUG) {
      // Wait for a command from the debugger.
      continue;
    }

    cycle(cpu, gpu, timer, debugger_state);
  }
}

static constexpr int VIEW_ID = 0;

void graphics_loop(CPU& cpu, GPU& gpu, DebuggerState& debugger_state) {
  Factory::Init();

  Display display("GBA Emulator", 1920, 1080);
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

  // RGB5A1 causes the depth buffer to use swizzling, which causes crashes on resize (bgfx::reset).
  // Using my fork of bgfx to fix it for now, issue is here:
  // https://github.com/bkaradzic/bgfx/issues/3344
  auto frameTexture = new Texture2D(
    FRAME_WIDTH,
    FRAME_HEIGHT,
    false,
    1,
    bgfx::TextureFormat::RGB5A1,
    BGFX_SAMPLER_MIN_POINT |
    BGFX_SAMPLER_MAG_POINT |
    BGFX_SAMPLER_MIP_POINT
  );

  std::function<void()> updateCallback = [&]() {};
  std::function<void()> renderCallback = [&]() {
    gui->NewFrame();

    // Get the display size from ImGui
    auto displaySize = ImGui::GetIO().DisplaySize;

    // Create a dockspace window that is the size of the display always 
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, displaySize.y));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin(
      "MainWindow",
      (bool*)0,
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_NoDecoration
    );
    ImGui::DockSpace(
      ImGui::GetID("MyDockSpace"),
      ImVec2(0, 0),
      ImGuiDockNodeFlags_PassthruCentralNode
    );
    ImGui::PopStyleVar();

    // Update the frame texture.
    frameTexture->Update(
      0,
      0,
      FRAME_WIDTH,
      FRAME_HEIGHT,
      gpu.frame_buffer,
      FRAME_BUFFER_SIZE_BYTES,
      FRAME_BUFFER_PITCH
    );

    if (ImGui::Begin("GBA Emulator")) {
      ImGui::Image(frameTexture->GetHandle(), ImVec2(240 * 2, 160 * 2));
    }
    ImGui::End();

    cpu_debugger_window(cpu, debugger_state);
    cpu_history_window(cpu, debugger_state);
    palette_debugger_window(cpu);
    sprite_debugger_window(cpu);
    ram_debugger_window(cpu);
    special_effects_debugger_window(cpu);
    window_debugger_window(cpu);
    bg_debugger_window(cpu);
    state_debugger_window(cpu);

    // Input handling.
    input_handle_key_detection(cpu, inputManager);

    ImGui::End();

    gui->EndFrame();

    graphics->Clear(VIEW_ID, 20, 20, 20, 255);
    graphics->Viewport(VIEW_ID, 0, 0, display.GetWidth(), display.GetHeight());
    graphics->Touch(VIEW_ID);

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

void start_cpu_loop(CPU& cpu, GPU& gpu, Timer& timer, DebuggerState& debugger_state) {
  while (!cpu.killSignal) {
    try {
      emulator_loop(cpu, gpu, timer, debugger_state);
    } catch (std::exception& e) {
      debug_print_cpu_state(cpu);
      std::cout << e.what() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  CPU cpu;
  GPU gpu;
  Timer timer;
  DebuggerState debugger_state;

  // Start with the debugger set to break straight away.
  debugger_state.mode = DEBUG;

  // Run CPU in a separate thread.
  std::thread cpu_thread(
    start_cpu_loop,
    std::ref(cpu),
    std::ref(gpu),
    std::ref(timer),
    std::ref(debugger_state)
  );

  // Run graphics in the main thread.
  graphics_loop(cpu, gpu, debugger_state);

  cpu_thread.join();

  return 0;
}
