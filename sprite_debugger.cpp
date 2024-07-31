#include <string>
#include "gpu.h"
#include "sprite_debugger.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

static Texture2D* sprite_texture = nullptr;
static uint16_t sprite_texture_buffer[64 * 64];
static int selected_sprite = 0;

static constexpr int SPRITE_PITCH = 64;
static constexpr int SPRITE_TEXTURE_BUFFER_SIZE = 64 * 64 * sizeof(uint16_t);
static constexpr int TILE_SIZE = 8;
static constexpr int HALF_TILE_SIZE = TILE_SIZE / 2;
static constexpr int TILE_4BPP_SIZE = 32;
static constexpr int TILE_8BPP_SIZE = 64;
static constexpr uint16_t ENABLE_PIXEL = 1 << 15;

void sprite_debugger_window(CPU& cpu) {
  if (ImGui::Begin("Sprite Debugger")) {
    ImGui::InputInt("Sprite ID", &selected_sprite);

    uint16_t* oam = (uint16_t*)cpu.ram.object_attribute_memory;
    uint16_t attr0 = oam[selected_sprite * 4];
    uint16_t attr1 = oam[selected_sprite * 4 + 1];
    uint16_t attr2 = oam[selected_sprite * 4 + 2];

    uint32_t obj_address = 0x07000000 + (selected_sprite * 8);
    ImGui::Text("Address: 0x%08X", obj_address);

    ImGui::Text("Attributes 0: 0x%04X", attr0);
    ImGui::Text("Attributes 1: 0x%04X", attr1);
    ImGui::Text("Attributes 2: 0x%04X", attr2);

    uint8_t obj_mode = (attr0 >> 10) & 0x3;
    switch (obj_mode) {
      case 0:
        ImGui::Text("Mode: Regular Sprite");
        break;
      case 1:
        ImGui::Text("Mode: Semi-Transparent");
        break;
      case 2:
        ImGui::Text("Mode: Window");
        break;
      case 3:
        ImGui::Text("Mode: Prohibited");
        break;
    }

    uint16_t x_coord = attr1 & 0x1FF;
    uint16_t y_coord = attr0 & 0xFF;

    ImGui::Text("X: %d", x_coord);
    ImGui::Text("Y: %d", y_coord);

    uint8_t priority = (attr2 >> 10) & 0x3;
    ImGui::Text("Priority: %d", priority);

    bool is_256_color_mode = attr0 & (1 << 13);
    const char* palette_mode = is_256_color_mode ? "256 colors" : "16 colors";
    ImGui::Text("Palette Mode: %s", palette_mode);

    std::string shape;
    uint8_t shape_enum = (attr0 & (3 << 14)) >> 14;
    switch (shape_enum) {
      case 0:
        shape = "Square";
        break;
      case 1:
        shape = "Horizontal";
        break;
      case 2:
        shape = "Vertical";
        break;
      case 3:
        shape = "Prohibited";
        break;
    }
    ImGui::Text("Shape: %s", shape.c_str());

    bool rotation_scaling = attr0 & (1 << 8);
    ImGui::Checkbox("Rotation/Scaling", &rotation_scaling);

    if (rotation_scaling) {
      bool double_size = attr0 & (1 << 9);
      ImGui::Checkbox("Double Size", &double_size);

      // Matrix
      uint8_t matrix_index = (attr1 & (0xF << 9)) >> 9;
      ImGui::Text("Matrix Index: %d", matrix_index);

      // Rotation / Scaling parameters
      int16_t pa = 0;
      int16_t pb = 0;
      int16_t pc = 0;
      int16_t pd = 0;
      gpu_get_obj_affine_params(cpu, attr1, pa, pb, pc, pd);

      // Values are fixed point 8.8 (8 bits integer, 8 bits fractional)
      // To convert to float, divide by 2^N where N is the number of fractional bits.
      float a = pa / 256.0f;
      float b = pb / 256.0f;
      float c = pc / 256.0f;
      float d = pd / 256.0f;

      ImGui::Text("pa: %f", a);
      ImGui::Text("pb: %f", b);
      ImGui::Text("pc: %f", c);
      ImGui::Text("pd: %f", d);
    } else {
      bool disable = attr0 & (1 << 9);
      ImGui::Checkbox("Disabled", &disable);

      bool horizontal_flip = attr1 & (1 << 12);
      bool vertical_flip = attr1 & (1 << 13);

      ImGui::Checkbox("Horizontal Flip", &horizontal_flip);
      ImGui::Checkbox("Vertical Flip", &vertical_flip);
    }

    uint16_t disp_cnt = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
    bool is_one_dimensional = disp_cnt & (1 << 6);
    ImGui::Checkbox("One Dimensional", &is_one_dimensional);

    uint8_t size_enum = (attr1 & (3 << 14)) >> 14;
    uint8_t width = 0;
    uint8_t height = 0;
    gpu_get_obj_size(shape_enum, size_enum, width, height);
    ImGui::Text("Size: %dx%d", width, height);

    uint16_t tile_base = attr2 & 0x3FF;
    if (is_256_color_mode) {
      // When in 256 color mode, only every other tile is used, and the first bit is ignored.
      tile_base >>= 1;
    }
    ImGui::Text("Tile Base: %d", tile_base);

    uint8_t palette_number = attr2 >> 12;
    if (!is_256_color_mode) {
      ImGui::Text("Palette Number: %d", palette_number);
    }

    if (sprite_texture == nullptr) {
      sprite_texture = new Texture2D(
        64,
        64,
        false,
        1,
        bgfx::TextureFormat::RGB5A1,
        BGFX_TEXTURE_NONE |
        BGFX_SAMPLER_MIN_POINT |
        BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT
      );
    }

    for (int i = 0; i < 64 * 64; i++) {
      sprite_texture_buffer[i] = 0;
    }

    uint16_t* sprite_palette_ram = is_256_color_mode
      ? (uint16_t*)(cpu.ram.palette_ram + 0x200)
      : (uint16_t*)(cpu.ram.palette_ram + 0x200 + palette_number * 32);

    uint8_t* vram = cpu.ram.video_ram;
    uint8_t* base_sprite_tile_ram = vram + 0x10000;

    uint8_t width_in_tiles = width / TILE_SIZE;
    uint8_t height_in_tiles = height / TILE_SIZE;
    uint8_t number_of_tiles = width_in_tiles * height_in_tiles;

    uint8_t tile_size = is_256_color_mode ? TILE_8BPP_SIZE : TILE_4BPP_SIZE;

    for (uint8_t tile_offset = 0; tile_offset < number_of_tiles; tile_offset++) {
      uint8_t col_idx = tile_offset % width_in_tiles;
      uint8_t row_idx = tile_offset / width_in_tiles;

      uint16_t map_offset = tile_base + tile_offset;
      if (!is_one_dimensional) {
        if (is_256_color_mode) {
          // TODO: Still unsure on how this is 16 and not 32...
          map_offset = tile_base + row_idx * 16 + col_idx;
        } else {
          map_offset = tile_base + row_idx * 32 + col_idx;
        }
      }

      uint8_t tile_offset_x = col_idx * TILE_SIZE;
      uint8_t tile_offset_y = row_idx * TILE_SIZE;

      uint8_t* current_tile = &base_sprite_tile_ram[map_offset * tile_size];
      if (is_256_color_mode) {
        for (int i = 0; i < tile_size; i++) {
          uint8_t palette_idx = current_tile[i];
          uint16_t color = sprite_palette_ram[palette_idx];

          if (color > 0) {
            color |= ENABLE_PIXEL;
          }

          uint16_t x = tile_offset_x + i % TILE_SIZE;
          uint16_t y = tile_offset_y + i / TILE_SIZE;

          sprite_texture_buffer[y * SPRITE_PITCH + x] = color;
        }
      } else {
        for (int i = 0; i < tile_size; i++) {
          uint8_t palette_indices = current_tile[i];
          uint16_t color_left = sprite_palette_ram[palette_indices & 0xF];
          uint16_t color_right = sprite_palette_ram[palette_indices >> 4];

          if (color_left > 0) {
            color_left |= ENABLE_PIXEL;
          }

          if (color_right > 0) {
            color_right |= ENABLE_PIXEL;
          }

          uint16_t x = tile_offset_x + 2 * (i % HALF_TILE_SIZE);
          uint16_t y = tile_offset_y + i / HALF_TILE_SIZE;

          sprite_texture_buffer[y * SPRITE_PITCH + x] = color_left;
          sprite_texture_buffer[y * SPRITE_PITCH + x + 1] = color_right;
        }
      }
    }

    sprite_texture->Update(
      0,
      0,
      64,
      64,
      sprite_texture_buffer,
      SPRITE_TEXTURE_BUFFER_SIZE,
      SPRITE_PITCH
    );

    ImGui::Image(sprite_texture->GetHandle(), ImVec2(64 * 2, 64 * 2));
  }
  ImGui::End();
}
