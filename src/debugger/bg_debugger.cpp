#include "bg_debugger.h"
#include "../gpu.h"

#include "3rdparty/zengine/ZEngine-Core/Rendering/Texture2D.h"
#include "3rdparty/zengine/ZEngine-Core/ImmediateUI/imgui-includes.h"

static int selected_bg = 0;
static Texture2D* bg_texture = nullptr;
static uint16_t bg_texture_buffer[1024 * 1024];
static int view_tile_x = 0;
static int view_tile_y = 0;
static bool debug_frame_select = false;

static constexpr int BG_TEXTURE_BUFFER_SIZE = 1024 * 1024 * sizeof(uint16_t);

float fixed_to_float(int32_t fixed_point) {
  // Convert to float first to preserve precision
  float result = (float)fixed_point;
  
  // Divide by 2^8 (256) to adjust for the 8-bit fractional part
  result /= 256.0f;
  
  return result;
}

void bg_debugger_window(CPU& cpu) {
  uint16_t disp_control_data = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  DisplayControl disp_control = *(DisplayControl*)&disp_control_data;

  if (ImGui::Begin("Background Debugger")) {
    ImGui::Text("Display Control:");
    ImGui::Text("Background Mode: %d", disp_control.background_mode);
    ImGui::Text("Display Frame Select: %d", disp_control.display_frame_select);

    bool hblank_interval_free = disp_control.hblank_interval_free;
    ImGui::Checkbox("HBlank Interval Free", &hblank_interval_free);

    bool one_dimensional_mapping = disp_control.one_dimensional_mapping;
    ImGui::Checkbox("One-Dimensional Mapping", &one_dimensional_mapping);

    bool force_blank = disp_control.force_blank;
    ImGui::Checkbox("Force Blank", &force_blank);

    bool display_bg0 = disp_control.display_bg0;
    ImGui::Checkbox("Display BG0", &display_bg0);

    bool display_bg1 = disp_control.display_bg1;
    ImGui::Checkbox("Display BG1", &display_bg1);

    bool display_bg2 = disp_control.display_bg2;
    ImGui::Checkbox("Display BG2", &display_bg2);

    bool display_bg3 = disp_control.display_bg3;
    ImGui::Checkbox("Display BG3", &display_bg3);

    bool display_obj = disp_control.display_obj;
    ImGui::Checkbox("Display OBJ", &display_obj);

    bool display_window0 = disp_control.display_window0;
    ImGui::Checkbox("Display Window 0", &display_window0);

    bool display_window1 = disp_control.display_window1;
    ImGui::Checkbox("Display Window 1", &display_window1);

    bool display_obj_window = disp_control.display_obj_window;
    ImGui::Checkbox("Display OBJ Window", &display_obj_window);
  }
  ImGui::End();

  if (ImGui::Begin("Background Visualiser")) {
    ImGui::Combo("Background", &selected_bg, "BG0\0BG1\0BG2\0BG3\0\0");

    switch (disp_control.background_mode) {
      case 0:
        ImGui::Text("Mode 0: 4 Backgrounds");
        break;
      case 1:
        ImGui::Text("Mode 1: 3 Backgrounds (BG0, BG1 - Mode 0, BG2 - Mode 2)");
        break;
      case 2:
        ImGui::Text("Mode 2: 2 Backgrounds (BG2, BG3 only)");
        break;
      case 3:
        ImGui::Text("Mode 3: 1 Background (Bitmap based, BG2 only, 32k color mode)");
        break;
      case 4:
        ImGui::Text("Mode 4: 2 Background (Bitmap based, BG2 only, 256 color mode)");
        break;
      case 5:
        ImGui::Text("Mode 5: 2 Backgrounds (Bitmap based, BG2 only, 32k color mode)");
        break;
    }

    uint8_t* bg_control_mem = ram_read_memory_from_io_registers_fast<REG_BG0_CONTROL>(cpu.ram);
    uint16_t const& bg_control_data = *(uint16_t*)(bg_control_mem + selected_bg * 2);
    ImGui::Text("BG Control: 0x%04X", bg_control_data);

    BackgroundControl const& bg_control = *(BackgroundControl*)&bg_control_data;
    ImGui::Text("Priority: %d", bg_control.priority);
    ImGui::Text("Char Base Block: %d", bg_control.char_base_block);
    ImGui::Text("Screen Base Block: %d", bg_control.screen_base_block);

    bool is_rotation_scaling = disp_control.background_mode >= 2 || (disp_control.background_mode == 1 && selected_bg == 2);
    switch (bg_control.screen_size) {
      case 0:
        ImGui::Text(is_rotation_scaling ? "Screen Size: 128x128" : "Screen Size: 256x256");
        break;
      case 1:
        ImGui::Text(is_rotation_scaling ? "Screen Size: 256x256" : "Screen Size: 512x256");
        break;
      case 2:
        ImGui::Text(is_rotation_scaling ? "Screen Size: 512x512" : "Screen Size: 256x512");
        break;
      case 3:
        ImGui::Text(is_rotation_scaling ? "Screen Size: 1024x1024" : "Screen Size: 512x512");
        break;
    }

    ImGui::Checkbox("Rotation / Scaling", &is_rotation_scaling);

    bool mosaic = bg_control.mosaic;
    ImGui::Checkbox("Mosaic", &mosaic);

    bool is_256_color_mode = bg_control.is_256_color_mode;
    ImGui::Checkbox("256 Color Mode", &is_256_color_mode);

    bool display_area_overflow = bg_control.display_area_overflow;
    ImGui::Checkbox("Display Area Overflow", &display_area_overflow);

    int32_t bg_offset_x = 0;
    int32_t bg_offset_y = 0;

    if (is_rotation_scaling && selected_bg >= 2) {
      bg_offset_x = selected_bg == 2 
        ? ram_read_word_from_io_registers_fast<REG_BG2_X_REF>(cpu.ram)
        : ram_read_word_from_io_registers_fast<REG_BG3_X_REF>(cpu.ram);
      bg_offset_y = selected_bg == 2 
        ? ram_read_word_from_io_registers_fast<REG_BG2_Y_REF>(cpu.ram)
        : ram_read_word_from_io_registers_fast<REG_BG3_Y_REF>(cpu.ram);

      // Convert from fixed point to floating point.
      // 8 fractional bits.
      // 19 integer bits.
      // 1 sign bit.
      float x = fixed_to_float(bg_offset_x);
      float y = fixed_to_float(bg_offset_y);

      ImGui::Text("BG Offset X: %f", x);
      ImGui::Text("BG Offset Y: %f", y);
    } else {
      uint8_t* bg_offset_x_mem = ram_read_memory_from_io_registers_fast<REG_BG0_X_OFFSET>(cpu.ram);
      uint8_t* bg_offset_y_mem = ram_read_memory_from_io_registers_fast<REG_BG0_Y_OFFSET>(cpu.ram);

      bg_offset_x = (int32_t)(*(int16_t*)(bg_offset_x_mem + selected_bg * 4)) & 0x1FF;
      bg_offset_y = (int32_t)(*(int16_t*)(bg_offset_y_mem + selected_bg * 4)) & 0x1FF;

      ImGui::Text("BG Offset X: %d", bg_offset_x);
      ImGui::Text("BG Offset Y: %d", bg_offset_y);
    }

    uint8_t* vram = cpu.ram.video_ram;
    uint16_t* palette_ram = (uint16_t*)cpu.ram.palette_ram;
    uint8_t* base_bg_tile_ram = vram + bg_control.char_base_block * 0x4000;
    uint16_t* base_screen_block_ram = (uint16_t*)(vram + bg_control.screen_base_block * 0x800);

    uint32_t width_in_tiles = 0;
    uint32_t height_in_tiles = 0;
    int width_in_pixels = 0;
    int height_in_pixels = 0;

    bool bitmap_mode = disp_control.background_mode >= 3;

    if (bitmap_mode && disp_control.background_mode != 5) {
      width_in_pixels = 240;
      height_in_pixels = 160;
    } else if (bitmap_mode) {
      width_in_pixels = 160;
      height_in_pixels = 128;
    } else {
      gpu_get_bg_size_in_tiles(
        is_rotation_scaling,
        bg_control.screen_size,
        width_in_tiles,
        height_in_tiles
      );

      width_in_pixels = width_in_tiles * TILE_SIZE;
      height_in_pixels = height_in_tiles * TILE_SIZE;
    }

    ImGui::Text("Width (px): %d", width_in_pixels);
    ImGui::Text("Height (px): %d", height_in_pixels);

    if (bg_texture == nullptr) {
      // Create the texture.
      bg_texture = new Texture2D(
        width_in_pixels,
        height_in_pixels,
        false,
        1,
        bgfx::TextureFormat::RGB5A1,
        BGFX_TEXTURE_NONE |
        BGFX_SAMPLER_MIN_POINT |
        BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT
      );
    } else {
      // Update the texture if the size has changed.
      if (bg_texture->GetWidth() != width_in_pixels || bg_texture->GetHeight() != height_in_pixels) {
        delete bg_texture;
        bg_texture = new Texture2D(
          width_in_pixels,
          height_in_pixels,
          false,
          1,
          bgfx::TextureFormat::RGB5A1,
          BGFX_TEXTURE_NONE |
          BGFX_SAMPLER_MIN_POINT |
          BGFX_SAMPLER_MAG_POINT |
          BGFX_SAMPLER_MIP_POINT
        );
      }
    }

    for (int i = 0; i < width_in_pixels * height_in_pixels; i++) {
      bg_texture_buffer[i] = 0;
    }

    if (bitmap_mode) {
      ImGui::Checkbox("Display Frame Select", &debug_frame_select);

      uint32_t frame_offset = debug_frame_select ? 256 * height_in_pixels : 0;
      for (int y = 0; y < height_in_pixels; y++) {
        for (int x = 0; x < width_in_pixels; x++) {
          if (disp_control.background_mode == 4) {
            // 256 colors, 8bpp.
            uint8_t palette_idx = vram[frame_offset + y * width_in_pixels + x];
            uint16_t color = palette_ram[palette_idx];
            bg_texture_buffer[y * width_in_pixels + x] = color | ENABLE_PIXEL;
          } else {
            // 32k colors, 16bpp.
            uint16_t color = ((uint16_t*)vram)[frame_offset + y * width_in_pixels + x];
            bg_texture_buffer[y * width_in_pixels + x] = color | ENABLE_PIXEL;
          }
        }
      }
    } else {
      uint8_t tile_size_bytes = bg_control.is_256_color_mode ? TILE_8BPP_BYTES : TILE_4BPP_BYTES;

      ImGui::InputInt("View Tile X", &view_tile_x);
      ImGui::InputInt("View Tile Y", &view_tile_y);

      for (int tile_y = 0; tile_y < height_in_tiles; tile_y++) {
        int tile_screen_y = tile_y * TILE_SIZE;
        for (int tile_x = 0; tile_x < width_in_tiles; tile_x++) {
          int tile_screen_x = tile_x * TILE_SIZE;
          if (is_rotation_scaling) {
            // 1 byte per screen entry, 256 colors, 8bpp (1 byte per pixel in tile).
            int screen_entry_idx = tile_y * width_in_tiles + tile_x;
            uint8_t tile_number = ((uint8_t*)base_screen_block_ram)[screen_entry_idx];
            uint8_t* tile_data = &base_bg_tile_ram[tile_number * TILE_8BPP_BYTES];
            for (int y = 0; y < TILE_SIZE; y++) {
              for (int x = 0; x < TILE_SIZE; x++) {
                uint8_t palette_idx = tile_data[y * TILE_SIZE + x];
                uint16_t color = palette_ram[palette_idx];
                if (color > 0) {
                  bg_texture_buffer[(tile_screen_y + y) * width_in_pixels + tile_screen_x + x] = color | ENABLE_PIXEL;
                }
              }
            }
          } else {
            // 2 bytes per screen entry, can be 16 (4bpp) or 256 colors (8bpp).
            int screen_block_idx = 0;

            if (width_in_tiles == height_in_tiles) {
              screen_block_idx = (tile_y / 32) * (width_in_tiles / 32) + (tile_x / 32);
            } else if (width_in_tiles > height_in_tiles) {
              screen_block_idx = tile_x / 32;
            } else {
              screen_block_idx = tile_y / 32;
            }

            int screen_entry_idx = screen_block_idx * 0x400 + (tile_y % 32) * 32 + (tile_x % 32);
            uint16_t screen_entry = base_screen_block_ram[screen_entry_idx];
            
            uint16_t tile_number = screen_entry & 0x3FF;
            uint8_t flip_mode = (screen_entry >> 10) & 0x3;
            bool horizontal_flip = flip_mode & 0x1;
            bool vertical_flip = flip_mode & 0x2;
            uint8_t palette_bank = (screen_entry >> 12) & 0xF;

            if (tile_x == view_tile_x && tile_y == view_tile_y) {
              ImGui::Text("Screen Entry: 0x%04X", screen_entry);
              ImGui::Text("Tile Number: %d", tile_number);
              ImGui::Text("Flip Mode: %d", flip_mode);
              ImGui::Text("Horizontal Flip: %d", horizontal_flip);
              ImGui::Text("Vertical Flip: %d", vertical_flip);
              ImGui::Text("Palette Bank: %d", palette_bank);
              ImGui::Text("Screen Entry Index: %d", screen_entry_idx);
            }

            uint8_t* tile_data = &base_bg_tile_ram[tile_number * tile_size_bytes];
            if (bg_control.is_256_color_mode) {
              for (int y = 0; y < TILE_SIZE; y++) {
                for (int x = 0; x < TILE_SIZE; x++) {
                  // TODO: This code path is untested.
                  uint8_t palette_idx = tile_data[y * TILE_SIZE + x];
                  uint16_t color = palette_ram[palette_idx];

                  uint8_t x_flip = horizontal_flip ? TILE_SIZE - x - 1 : x;
                  uint8_t y_flip = vertical_flip ? TILE_SIZE - y - 1 : y;

                  if (color > 0) {
                    bg_texture_buffer[(tile_screen_y + y_flip) * width_in_pixels + tile_screen_x + x_flip] = color | ENABLE_PIXEL;
                  }
                }
              }
            } else {
              for (int y = 0; y < TILE_SIZE; y++) {
                for (int x = 0; x < HALF_TILE_SIZE; x++) {
                  uint8_t palette_indices = tile_data[y * HALF_TILE_SIZE + x];
                  uint8_t palette_idx_left = palette_indices & 0xF;
                  uint8_t palette_idx_right = (palette_indices >> 4) & 0xF;

                  uint8_t x_flip = horizontal_flip ? HALF_TILE_SIZE - x - 1 : x;
                  uint8_t y_flip = vertical_flip ? TILE_SIZE - y - 1 : y;

                  uint16_t color_left = palette_ram[palette_bank * 16 + palette_idx_left];
                  if (color_left > 0) {
                    bg_texture_buffer[(tile_screen_y + y_flip) * width_in_pixels + tile_screen_x + x_flip * 2] = color_left | ENABLE_PIXEL;
                  }

                  uint16_t color_right = palette_ram[palette_bank * 16 + palette_idx_right];
                  if (color_right > 0) {
                    bg_texture_buffer[(tile_screen_y + y_flip) * width_in_pixels + tile_screen_x + x_flip * 2 + 1] = color_right | ENABLE_PIXEL;
                  }
                }
              }
            }
          }
        }
      }
    }

    bg_texture->Update(
      0,
      0,
      width_in_pixels,
      height_in_pixels,
      bg_texture_buffer,
      BG_TEXTURE_BUFFER_SIZE,
      width_in_pixels * sizeof(uint16_t)
    );

    ImGui::Image(
      bg_texture->GetHandle(),
      ImVec2(width_in_pixels, height_in_pixels)
    );
  }
  ImGui::End();
}
