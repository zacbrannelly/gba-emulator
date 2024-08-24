#include "ram_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

enum class DataType {
  BYTE = 0,
  HALFWORD = 1,
  WORD = 2
};

static uint32_t ram_debugger_address = 0;
static int ram_debugger_dtype = 0;
static uint32_t ram_debugger_length = 16;
static uint32_t stack_debugger_length = 16;

void ram_debugger_window(CPU& cpu) {
  if (ImGui::Begin("RAM Debugger")) {
    ImGui::InputScalar("RAM Address", ImGuiDataType_U32, &ram_debugger_address, 0, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::Combo("Data Type", &ram_debugger_dtype, "Byte\0Halfword\0Word\0\0");
    ImGui::InputScalar("Data Length", ImGuiDataType_U32, &ram_debugger_length, 0, 0, "%d", ImGuiInputTextFlags_CharsDecimal);

    ImGui::BeginListBox("RAM Data", ImVec2(0, 200 + ram_debugger_length * 20));
    for (int i = 0; i < ram_debugger_length; i++) {
      uint32_t address = ram_debugger_address + i * (1 << ram_debugger_dtype);
      uint32_t data = 0;
      switch ((DataType)ram_debugger_dtype) {
        case DataType::BYTE:
          data = ram_read_byte_direct(cpu.ram, address);
          ImGui::Text("0x%08X: 0x%02X", address, data);
          break;
        case DataType::HALFWORD:
          data = ram_read_half_word_direct(cpu.ram, address);
          ImGui::Text("0x%08X: 0x%04X", address, data);
          break;
        case DataType::WORD:
          data = ram_read_word_direct(cpu.ram, address);
          ImGui::Text("0x%08X: 0x%08X", address, data);
          break;
      }
    }
    ImGui::EndListBox();
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
