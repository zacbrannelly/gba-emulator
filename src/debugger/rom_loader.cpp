#include "rom_loader.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

void rom_loader_window(CPU& cpu) {
  if (ImGui::Begin("ROM Loader")) {
    static char rom_path[256] = "";
    ImGui::InputText("ROM Path", rom_path, IM_ARRAYSIZE(rom_path));

    if (ImGui::Button("Load ROM")) {
      ram_load_rom(cpu.ram, rom_path);
    }
  }
  ImGui::End();
}
