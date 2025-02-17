#include "palette_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

using namespace ZEngine;

static Texture2D* bg_palette_texture = nullptr;
static uint16_t bg_palette_texture_buffer[256];

static Texture2D* sprite_palette_texture = nullptr;
static uint16_t sprite_palette_texture_buffer[256];

void palette_debugger_window(CPU& cpu) {
  if (ImGui::Begin("Palette Debugger")) {
    if (bg_palette_texture == nullptr) {
      bg_palette_texture = new Texture2D(
        16,
        16,
        false,
        1,
        bgfx::TextureFormat::RGB5A1,
        BGFX_TEXTURE_NONE |
        BGFX_SAMPLER_MIN_POINT |
        BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT
      );

      sprite_palette_texture = new Texture2D(
        16,
        16,
        false,
        1,
        bgfx::TextureFormat::RGB5A1,
        BGFX_TEXTURE_NONE |
        BGFX_SAMPLER_MIN_POINT |
        BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT
      );
    }

    uint16_t* palette_ram = (uint16_t*)cpu.ram.palette_ram;
    uint16_t* sprite_palette_ram = (uint16_t*)(cpu.ram.palette_ram + 0x200);

    for (int i = 0; i < 256; i++) {
      uint16_t color = palette_ram[i] | (1 << 15);
      bg_palette_texture_buffer[i] = color;
    }
    bg_palette_texture->Update(
      0,
      0,
      16,
      16,
      bg_palette_texture_buffer,
      256 * sizeof(uint16_t),
      16 * sizeof(uint16_t)
    );

    for (int i = 0; i < 256; i++) {
      uint16_t color = sprite_palette_ram[i] | (1 << 15);
      sprite_palette_texture_buffer[i] = color;
    }
    sprite_palette_texture->Update(
      0,
      0,
      16,
      16,
      sprite_palette_texture_buffer,
      256 * sizeof(uint16_t),
      16 * sizeof(uint16_t)
    );
    
    ImGui::Image(bg_palette_texture->GetHandle(), ImVec2(256, 256));
    ImGui::SameLine();
    ImGui::Image(sprite_palette_texture->GetHandle(), ImVec2(256, 256));
  }
  ImGui::End();
}