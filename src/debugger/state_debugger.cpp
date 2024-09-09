#include "state_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

#include <filesystem>
#include <vector>
#include <set>
#include <sstream>

static bool existing_states_loaded = false;
static std::set<std::string> state_names;
static char* state_name = new char[256];
static char* selected_state = new char[256];

void state_debugger_window(CPU& cpu) {
  // Load the existing states from the file system.
  if (!existing_states_loaded) {
    for (const auto& entry : std::filesystem::directory_iterator("states")) {
      if (entry.path().extension() != ".state") continue;
      state_names.insert(entry.path().filename().string());
    }
    existing_states_loaded = true;
  }

  if (ImGui::Begin("State Debugger")) {
    ImGui::InputText("State Name", state_name, 256);
    ImGui::SameLine();

    if (ImGui::Button("Save State")) {
      // Skip saving if the state name is empty.
      std::string state_name_str(state_name);
      if (state_name_str.empty()) return;

      // Save the state to the file system.
      std::stringstream ss;
      ss << "states/" << state_name_str << ".state";
      save_state(cpu, ss.str());

      // Cache the state name for the load state dropdown.
      ss = std::stringstream();
      ss << state_name_str << ".state";
      state_names.insert(ss.str());
    }

    // Load state dropdown.
    if (ImGui::BeginCombo("Load State##Combo", selected_state)) {
      for (const auto& state : state_names) {
        if (ImGui::Selectable(state.c_str())) {
          strcpy(selected_state, state.c_str());
        }
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();

    if (ImGui::Button("Load State##Button")) {
      // Skip loading if the state name is empty.
      std::string state_name_str(selected_state);
      if (state_name_str.empty()) return;

      // Load the state from the file system.
      std::stringstream ss;
      ss << "states/" << state_name_str;
      load_state(cpu, ss.str());
    }
  }
  ImGui::End();
}
