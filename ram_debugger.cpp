#include "ram_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

static uint32_t ram_debugger_address = 0;
static uint32_t stack_debugger_length = 16;

void ram_debugger_window(CPU& cpu) {
  if (ImGui::Begin("RAM Debugger")) {
    ImGui::InputScalar("RAM Address", ImGuiDataType_U32, &ram_debugger_address, 0, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);

    uint8_t byte = ram_read_byte_direct(cpu.ram, ram_debugger_address);
    ImGui::Text("Byte: 0x%02X", byte);

    uint16_t halfword = ram_read_half_word_direct(cpu.ram, ram_debugger_address);
    ImGui::Text("Halfword: 0x%04X", halfword);

    uint32_t word = ram_read_word_direct(cpu.ram, ram_debugger_address);
    ImGui::Text("Word: 0x%08X", word);
  }
  ImGui::End();

  if (ImGui::Begin("Stack Debugger")) {
    ImGui::InputScalar("Stack Length", ImGuiDataType_U32, &stack_debugger_length, 0, 0, "%d", ImGuiInputTextFlags_CharsDecimal);
    for (int i = 0; i < stack_debugger_length; i++) {
      uint32_t stack_address = cpu.get_register_value(SP) + i * 4;
      uint32_t word = ram_read_word_direct(cpu.ram, stack_address);
      ImGui::Text("0x%08X: 0x%08X", stack_address, word);
    }
  }
  ImGui::End();
}
