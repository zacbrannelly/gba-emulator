#include "special_effects_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

void special_effects_debugger_window(CPU& cpu) {
  if (ImGui::Begin("Special Effects Debugger")) {
    uint16_t special_effects_control = ram_read_half_word_from_io_registers_fast<REG_BLDCNT>(cpu.ram);
    uint16_t blend_alpha_coefficients = ram_read_half_word_from_io_registers_fast<REG_BLDALPHA>(cpu.ram);
    uint16_t brightness_coefficients = ram_read_half_word_from_io_registers_fast<REG_BLDY>(cpu.ram);
    uint8_t special_effects_mode = (special_effects_control >> 6) & 0x3;

    ImGui::Text("BLDCNT: 0x%04X", special_effects_control);
    ImGui::Text("Special Effects Mode: %d", special_effects_mode);

    switch (special_effects_mode) {
      case 1: {
        ImGui::Text("BLDALPHA: 0x%04X", blend_alpha_coefficients);
        uint8_t alpha_a = blend_alpha_coefficients & 0x1F;
        uint8_t alpha_b = (blend_alpha_coefficients >> 8) & 0x1F;
        ImGui::Text("Alpha A: %d", alpha_a);
        ImGui::Text("Alpha B: %d", alpha_b);
        break;
      }
      case 2:
      case 3:
        ImGui::Text("BLDY: 0x%04X", brightness_coefficients);
        break;
    }

    bool bg0_target_1 = special_effects_control & 0x1;
    bool bg1_target_1 = special_effects_control & 0x2;
    bool bg2_target_1 = special_effects_control & 0x4;
    bool bg3_target_1 = special_effects_control & 0x8;
    bool obj_target_1 = special_effects_control & 0x10;
    bool bd_target_1 = special_effects_control & 0x20;

    ImGui::Checkbox("BG0 Target 1", &bg0_target_1);
    ImGui::Checkbox("BG1 Target 1", &bg1_target_1);
    ImGui::Checkbox("BG2 Target 1", &bg2_target_1);
    ImGui::Checkbox("BG3 Target 1", &bg3_target_1);
    ImGui::Checkbox("OBJ Target 1", &obj_target_1);
    ImGui::Checkbox("Backdrop Target 1", &bd_target_1);

    bool bg0_target_2 = special_effects_control & 0x100;
    bool bg1_target_2 = special_effects_control & 0x200;
    bool bg2_target_2 = special_effects_control & 0x400;
    bool bg3_target_2 = special_effects_control & 0x800;
    bool obj_target_2 = special_effects_control & 0x1000;
    bool bd_target_2 = special_effects_control & 0x2000;

    ImGui::Checkbox("BG0 Target 2", &bg0_target_2);
    ImGui::Checkbox("BG1 Target 2", &bg1_target_2);
    ImGui::Checkbox("BG2 Target 2", &bg2_target_2);
    ImGui::Checkbox("BG3 Target 2", &bg3_target_2);
    ImGui::Checkbox("OBJ Target 2", &obj_target_2);
    ImGui::Checkbox("Backdrop Target 2", &bd_target_2);

    ImGui::End();
  }
}