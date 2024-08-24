#include "window_debugger.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

void window_debugger_window(CPU& cpu) {
  if (ImGui::Begin("Window Debugger")) {
    uint16_t disp_cnt = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
    bool window_0_enabled = disp_cnt & (1 << 13);
    bool window_1_enabled = disp_cnt & (1 << 14);
    bool obj_window_enabled = disp_cnt & (1 << 15);

    ImGui::Checkbox("Window 0 Enabled", &window_0_enabled);
    ImGui::Checkbox("Window 1 Enabled", &window_1_enabled);
    ImGui::Checkbox("OBJ Window Enabled", &obj_window_enabled);

    uint16_t win_inside = ram_read_half_word_from_io_registers_fast<REG_WINDOW_INSIDE>(cpu.ram);
    bool win0_bg0 = win_inside & 1;
    bool win0_bg1 = win_inside & (1 << 1);
    bool win0_bg2 = win_inside & (1 << 2);
    bool win0_bg3 = win_inside & (1 << 3);
    bool win0_obj = win_inside & (1 << 4);
    bool win0_color_special_effects = win_inside & (1 << 5);

    bool win1_bg0 = win_inside & (1 << 8);
    bool win1_bg1 = win_inside & (1 << 9);
    bool win1_bg2 = win_inside & (1 << 10);
    bool win1_bg3 = win_inside & (1 << 11);
    bool win1_obj = win_inside & (1 << 12);
    bool win1_color_special_effects = win_inside & (1 << 13);

    ImGui::Text("Window 0 Inside:");
    ImGui::Checkbox("BG0", &win0_bg0);
    ImGui::Checkbox("BG1", &win0_bg1);
    ImGui::Checkbox("BG2", &win0_bg2);
    ImGui::Checkbox("BG3", &win0_bg3);
    ImGui::Checkbox("OBJ", &win0_obj);
    ImGui::Checkbox("Color Special Effects", &win0_color_special_effects);

    ImGui::Text("Window 1 Inside:");
    ImGui::Checkbox("BG0", &win1_bg0);
    ImGui::Checkbox("BG1", &win1_bg1);
    ImGui::Checkbox("BG2", &win1_bg2);
    ImGui::Checkbox("BG3", &win1_bg3);
    ImGui::Checkbox("OBJ", &win1_obj);
    ImGui::Checkbox("Color Special Effects", &win1_color_special_effects);

    uint16_t win_outside = ram_read_half_word_from_io_registers_fast<REG_WINDOW_OUTSIDE>(cpu.ram);
    bool win_outside_bg0 = win_outside & 1;
    bool win_outside_bg1 = win_outside & (1 << 1);
    bool win_outside_bg2 = win_outside & (1 << 2);
    bool win_outside_bg3 = win_outside & (1 << 3);
    bool win_outside_obj = win_outside & (1 << 4);
    bool win_outside_color_special_effects = win_outside & (1 << 5);

    ImGui::Text("Outside of Windows:");
    ImGui::Checkbox("BG0", &win_outside_bg0);
    ImGui::Checkbox("BG1", &win_outside_bg1);
    ImGui::Checkbox("BG2", &win_outside_bg2);
    ImGui::Checkbox("BG3", &win_outside_bg3);
    ImGui::Checkbox("OBJ", &win_outside_obj);
    ImGui::Checkbox("Color Special Effects", &win_outside_color_special_effects);

    bool obj_win_bg0 = win_outside & (1 << 8);
    bool obj_win_bg1 = win_outside & (1 << 9);
    bool obj_win_bg2 = win_outside & (1 << 10);
    bool obj_win_bg3 = win_outside & (1 << 11);
    bool obj_win_obj = win_outside & (1 << 12);
    bool obj_win_color_special_effects = win_outside & (1 << 13);

    ImGui::Text("Inside of OBJ Window:");
    ImGui::Checkbox("BG0", &obj_win_bg0);
    ImGui::Checkbox("BG1", &obj_win_bg1);
    ImGui::Checkbox("BG2", &obj_win_bg2);
    ImGui::Checkbox("BG3", &obj_win_bg3);
    ImGui::Checkbox("OBJ", &obj_win_obj);
    ImGui::Checkbox("Color Special Effects", &obj_win_color_special_effects);
  }
  ImGui::End();
}
