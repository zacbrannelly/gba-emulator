#include "rom_loader.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>

static std::vector<std::string> recently_loaded_roms;
static bool recently_loaded_roms_loaded = false;

void rom_loader_window(CPU& cpu) {
  // Load recently loaded roms from a file.
  if (!recently_loaded_roms_loaded) {
    std::ifstream in("recent_roms.txt");
    std::string line;
    while (std::getline(in, line)) {
      recently_loaded_roms.push_back(line);
    }
    recently_loaded_roms_loaded = true;
  }

  if (ImGui::Begin("ROM Loader")) {
    static char rom_path[256] = "";
    ImGui::InputText("ROM Path", rom_path, IM_ARRAYSIZE(rom_path));

    if (ImGui::Button("Load ROM")) {
      // Load the rom into RAM.
      ram_load_rom(cpu.ram, rom_path);

      // Write the recently loaded roms to a file.
      if (std::find(recently_loaded_roms.begin(), recently_loaded_roms.end(), rom_path) == recently_loaded_roms.end())
        recently_loaded_roms.push_back(rom_path);

      std::ofstream out("recent_roms.txt");
      for (const auto& rom : recently_loaded_roms) {
        out << rom << std::endl;
      }
    }

    if (ImGui::BeginListBox("Recently Loaded ROMs")) {
      for (const auto& rom : recently_loaded_roms) {
        if (ImGui::Selectable(rom.c_str())) {
          strcpy(rom_path, rom.c_str());
        }
      }
      ImGui::EndListBox();
    }
  }
  ImGui::End();
}
