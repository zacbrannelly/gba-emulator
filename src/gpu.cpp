#include "gpu.h"
#include "debug.h"
#include <cstring>

static constexpr uint16_t REG_LCD_STATUS_HBLANK_FLAG = 1 << 1;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_FLAG = 1 << 2;
static constexpr uint16_t REG_LCD_STATUS_VBLANK_INTERRUPT_ENABLE = 1 << 3;
static constexpr uint16_t REG_LCD_STATUS_HBLANK_INTERRUPT_ENABLE = 1 << 4;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_INTERRUPT_ENABLE = 1 << 5;

void gpu_init(CPU& cpu, GPU& gpu) {
  // Initialize the frame buffer to white.
  for (uint32_t i = 0; i < FRAME_BUFFER_SIZE; i++) {
    gpu.frame_buffer[i] = 0xFFFF;
  }

  // TODO: Initialize the GPU registers.
}

inline uint16_t gpu_get_backdrop_color(CPU& cpu) {
  uint16_t* bg_palette_ram = (uint16_t*)(cpu.ram.palette_ram);
  uint16_t backdrop_color = bg_palette_ram[0];
  return backdrop_color > 0 ? backdrop_color | ENABLE_PIXEL : ENABLE_PIXEL;
}

inline void gpu_clear_scanline_buffers(GPU& gpu) {
  // Reset layer buffers.
  memset(gpu.scanline_buffer, 0, FRAME_WIDTH * sizeof(uint16_t));
  memset(gpu.scanline_special_effects_buffer, 0, FRAME_WIDTH * sizeof(uint16_t));
  memset(gpu.scanline_obj_window_buffer, 0, FRAME_WIDTH * sizeof(bool));
  memset(gpu.scanline_semi_transparent_buffer, 0, FRAME_WIDTH * sizeof(bool));
  for (int x = 0; x < FRAME_WIDTH; x++) {
    for (int priority = 0; priority < 4; priority++) {
      for (int pixel_source = 0; pixel_source < 6; pixel_source++) {
        gpu.scanline_by_priority_and_pixel_source[x][priority][pixel_source] = 0;
      }
    }
  }
}

inline void gpu_apply_window_effects(CPU& cpu, GPU& gpu) {
  uint16_t disp_cnt = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  uint16_t outside_window = ram_read_half_word_from_io_registers_fast<REG_WINDOW_OUTSIDE>(cpu.ram);

  bool layer_enabled_outside_window[6] = {
    // BG0
    (outside_window & 1) > 0,
    // BG1
    (outside_window & (1 << 1)) > 0,
    // BG2
    (outside_window & (1 << 2)) > 0,
    // BG3
    (outside_window & (1 << 3)) > 0,
    // OBJ
    (outside_window & (1 << 4)) > 0,
  };

  bool layer_enabled_in_obj_window[6] = {
    // BG0
    (outside_window & (1 << 8)) > 0,
    // BG1
    (outside_window & (1 << 9)) > 0,
    // BG2
    (outside_window & (1 << 10)) > 0,
    // BG3
    (outside_window & (1 << 11)) > 0,
    // OBJ
    (outside_window & (1 << 12)) > 0
  };

  // TODO: Window 0

  // TODO: Window 1

  // OBJ Window
  bool obj_window_enabled = disp_cnt & (1 << 15);
  if (!obj_window_enabled) return;

  for (int i = 0; i < FRAME_WIDTH; i++) {
    bool pixel_in_obj_window = gpu.scanline_obj_window_buffer[i];
    uint16_t special_effect_color = gpu.scanline_special_effects_buffer[i];

    if (!pixel_in_obj_window) {
      // Conditionally disable pixels outside the OBJ window.
      for (int priority = 0; priority < 4; priority++) {
        for (int pixel_source = 0; pixel_source < 5; pixel_source++) {
          if (gpu.scanline_by_priority_and_pixel_source[i][priority][pixel_source] > 0 && !layer_enabled_outside_window[pixel_source]) {
            gpu.scanline_by_priority_and_pixel_source[i][priority][pixel_source] = 0;
          }
        }
      }
    } else {
      // Conditionally disable pixels inside the OBJ window.
      for (int priority = 0; priority < 4; priority++) {
        for (int pixel_source = 0; pixel_source < 5; pixel_source++) {
          if (gpu.scanline_by_priority_and_pixel_source[i][priority][pixel_source] > 0 && !layer_enabled_in_obj_window[pixel_source]) {
            gpu.scanline_by_priority_and_pixel_source[i][priority][pixel_source] = 0;
          }
        }
      }
    }
  }
}

inline void gpu_apply_special_effects(CPU& cpu, GPU& gpu) {
  uint16_t special_effects_control = ram_read_half_word_from_io_registers_fast<REG_BLDCNT>(cpu.ram);
  uint8_t special_effects_mode = (special_effects_control >> 6) & 0x3;
  bool target_1[6] = {
    // BG0
    (special_effects_control & 0x1) > 0,
    // BG1
    (special_effects_control & 0x2) > 0,
    // BG2
    (special_effects_control & 0x4) > 0,
    // BG3
    (special_effects_control & 0x8) > 0,
    // OBJ
    (special_effects_control & 0x10) > 0,
    // Backdrop
    (special_effects_control & 0x20) > 0
  };
  bool target_2[6] = {
    (special_effects_control & 0x100) > 0,
    (special_effects_control & 0x200) > 0,
    (special_effects_control & 0x400) > 0,
    (special_effects_control & 0x800) > 0,
    (special_effects_control & 0x1000) > 0,
    (special_effects_control & 0x2000) > 0
  };

  uint16_t backdrop_color = gpu_get_backdrop_color(cpu);

  // TODO: Special effects mode is ignored on pixels that contain semi-transparent pixels from OBJ sprites.
  switch (special_effects_mode) {
    case 0: {
      // No special effects
      break;
    }
    case 1: {
      // Alpha Blending
      uint16_t blend_alpha_coefficients = ram_read_half_word_from_io_registers_fast<REG_BLDALPHA>(cpu.ram);
      uint8_t alpha_a = blend_alpha_coefficients & 0x1F;
      uint8_t alpha_b = (blend_alpha_coefficients >> 8) & 0x1F;

      if (alpha_a > 16) {
        alpha_a = 16;
      }
      if (alpha_b > 16) {
        alpha_b = 16;
      }

      float alpha_a_normalized = alpha_a / 16.0f;
      float alpha_b_normalized = alpha_b / 16.0f;

      for (int i = 0; i < FRAME_WIDTH; ++i) {
        PixelSource target_1_source = PIXEL_SOURCE_BACKDROP;
        uint16_t target_1_color = 0;

        PixelSource target_2_source = PIXEL_SOURCE_BACKDROP;
        uint16_t target_2_color = 0;

        // Get top most pixel and it's source.
        for (int j = 0; j < 4; j++) {
          auto color_by_priority_and_pixel_source = gpu.scanline_by_priority_and_pixel_source[i][j];
          for (int k = 4; k >= 0; k--) {
            uint16_t color = color_by_priority_and_pixel_source[k];
            if (color > 0) {
              if (target_1_color == 0) {
                // Find top most pixel for Target 1.
                target_1_color = color;
                target_1_source = (PixelSource)k;
              } else if (target_2_color == 0) {
                if ((PixelSource)k == PIXEL_SOURCE_OBJ && target_1_source == PIXEL_SOURCE_OBJ) {
                  // Skip OBJ since we already have the top most OBJ pixel.
                  continue;
                }
                // Find 2nd top most pixel for Target 2.
                target_2_color = color;
                target_2_source = (PixelSource)k;
                break;
              }
            }
          }
          if (target_2_color > 0) break;
        }

        // Skip if Target 1 is the backdrop, no layer to blend with.
        if (target_1_source == PIXEL_SOURCE_BACKDROP) continue;

        // Make sure to use the backdrop color if the target is the backdrop.
        if (target_2_source == PIXEL_SOURCE_BACKDROP) {
          target_2_color = backdrop_color;
        }

        // Only blend if the OBJ layer is semi-transparent.
        bool blending_with_obj = target_1_source == PIXEL_SOURCE_OBJ || target_2_source == PIXEL_SOURCE_OBJ;
        if (blending_with_obj) {
          if (!gpu.scanline_semi_transparent_buffer[i]) {
            continue;
          }

          // TODO: If I disable the following code, the bios screen looks correct.
          // TODO: Figure out why.
          // OBJ layer is semi-transparent, make sure the OBJ layer is first target.
          // if (target_2_source == PIXEL_SOURCE_OBJ) {
          //   // Swap the targets.
          //   uint16_t temp_color = target_1_color;
          //   target_1_color = target_2_color;
          //   target_2_color = temp_color;

          //   PixelSource temp_source = target_1_source;
          //   target_1_source = target_2_source;
          //   target_2_source = temp_source;
          // }
        } 
        // If not a semi-transparent OBJ, make sure target 1 is enabled for blending.
        else if (!target_1[target_1_source]) continue;
          
        // Always check if target 2 is enabled for blending.
        if (!target_2[target_2_source]) continue;

        uint8_t target_b_r = (target_2_color & 0x1F);
        uint8_t target_b_g = ((target_2_color >> 5) & 0x1F);
        uint8_t target_b_b = ((target_2_color >> 10) & 0x1F);

        uint8_t target_a_r = (target_1_color & 0x1F);
        uint8_t target_a_g = ((target_1_color >> 5) & 0x1F);
        uint8_t target_a_b = ((target_1_color >> 10) & 0x1F);

        uint8_t r = (uint8_t)(alpha_a_normalized * target_a_r + alpha_b_normalized * target_b_r);
        uint8_t g = (uint8_t)(alpha_a_normalized * target_a_g + alpha_b_normalized * target_b_g);
        uint8_t b = (uint8_t)(alpha_a_normalized * target_a_b + alpha_b_normalized * target_b_b);

        // Clip the intensity values.
        if (r > 0x1F) {
          r = 0x1F;
        }
        if (g > 0x1F) {
          g = 0x1F;
        }
        if (b > 0x1F) {
          b = 0x1F;
        }

        uint16_t blended_color = r | (g << 5) | (b << 10);
        if (blended_color > 0) {
          gpu.scanline_special_effects_buffer[i] = blended_color | ENABLE_PIXEL;
        }
      }
      break;
    }
    case 2: {
      // Brightness Increase Effect
      uint8_t effect_coefficients = ram_read_byte_from_io_registers_fast<REG_BLDY>(cpu.ram);
      if (effect_coefficients > 16) {
        effect_coefficients = 16;
      }
      float effect_coefficients_normalized = effect_coefficients / 16.0f;

      for (int i = 0; i < FRAME_WIDTH; ++i) {
        PixelSource target_1_source = PIXEL_SOURCE_BACKDROP;
        uint16_t target_1_color = 0;

        // Get top most pixel and it's source.
        for (int priority = 0; priority < 4; priority++) {
          auto color_by_priority_and_pixel_source = gpu.scanline_by_priority_and_pixel_source[i][priority];

          for (int pixel_source = 4; pixel_source >= 0; pixel_source--) {
            uint16_t color = color_by_priority_and_pixel_source[pixel_source];
            if (color > 0) {
              if (target_1_color == 0) {
                // Find top most pixel for Target 1.
                target_1_color = color;
                target_1_source = (PixelSource)pixel_source;
                break;
              }
            }
          }
          if (target_1_color > 0) break;
        }

        if (target_1_source == PIXEL_SOURCE_BACKDROP) {
          target_1_color = backdrop_color;
        }

        // Check if the layer is enabled for the target.
        if (!target_1[target_1_source]) continue;

        uint8_t target_a_r = (target_1_color & 0x1F);
        uint8_t target_a_g = ((target_1_color >> 5) & 0x1F);
        uint8_t target_a_b = ((target_1_color >> 10) & 0x1F);

        uint8_t r = target_a_r + (0x1F - target_a_r) * effect_coefficients_normalized;
        uint8_t g = target_a_g + (0x1F - target_a_g) * effect_coefficients_normalized;
        uint8_t b = target_a_b + (0x1F - target_a_b) * effect_coefficients_normalized;

        // Clip the intensity values.
        if (r > 0x1F) {
          r = 0x1F;
        }
        if (g > 0x1F) {
          g = 0x1F;
        }
        if (b > 0x1F) {
          b = 0x1F;
        }

        uint16_t blended_color = r | (g << 5) | (b << 10);
        if (blended_color > 0) {
          gpu.scanline_special_effects_buffer[i] = blended_color | ENABLE_PIXEL;
        }
      }
      break;
    }
    case 3: {
      // TODO: Brightness Decrease Effect
      break;
    }
  }
}

inline void gpu_apply_window_to_special_effects(CPU& cpu, GPU& gpu) {
  uint16_t disp_cnt = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  uint16_t outside_window = ram_read_half_word_from_io_registers_fast<REG_WINDOW_OUTSIDE>(cpu.ram);
  bool sfx_enabled_outside_window = (outside_window & (1 << 5)) > 0;
  bool sfx_enabled_in_obj_window = (outside_window & (1 << 13)) > 0;

  // TODO: Window 0

  // TODO: Window 1

  // OBJ Window
  bool obj_window_enabled = disp_cnt & (1 << 15);
  if (!obj_window_enabled) return;

  for (int i = 0; i < FRAME_WIDTH; i++) {
    bool pixel_in_obj_window = gpu.scanline_obj_window_buffer[i];
    uint16_t special_effect_color = gpu.scanline_special_effects_buffer[i];

    if (!pixel_in_obj_window) {
      // Conditionally disable special effects outside the OBJ window.
      if (special_effect_color > 0 && !sfx_enabled_outside_window) {
        gpu.scanline_special_effects_buffer[i] = 0;
      }
    } else {
      // Conditionally disable special effects inside the OBJ window.
      if (special_effect_color > 0 && !sfx_enabled_in_obj_window) {
        gpu.scanline_special_effects_buffer[i] = 0;
      }
    }
  }
}

inline void gpu_resolve_scanline_buffer(CPU& cpu, GPU& gpu) {
  for (int x = 0; x < FRAME_WIDTH; x++) {
    uint16_t special_effects_color = gpu.scanline_special_effects_buffer[x];
    if (special_effects_color > 0) {
      gpu.scanline_buffer[x] = special_effects_color;
      continue;
    }

    auto pixel_priority_map = gpu.scanline_by_priority_and_pixel_source[x];
    for (int priority = 3; priority >= 0; priority--) {
      for (int pixel_source = 4; pixel_source >= 0; pixel_source--) {
        if (pixel_priority_map[priority][pixel_source] > 0) {
          gpu.scanline_buffer[x] = pixel_priority_map[priority][pixel_source];
          break;
        }
      }
    }
  }
}

void gpu_render_bg_layer(CPU& cpu, GPU& gpu, uint8_t scanline) {
  uint16_t disp_cnt_data = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  DisplayControl const& disp_cnt = *(DisplayControl*)&disp_cnt_data;

  uint8_t* vram = cpu.ram.video_ram;
  uint16_t* palette_ram = (uint16_t*)(cpu.ram.palette_ram);
  uint8_t* bg_control_mem = ram_read_memory_from_io_registers_fast<REG_BG0_CONTROL>(cpu.ram);

  bool display_bg[4] = {
    disp_cnt.display_bg0,
    disp_cnt.display_bg1,
    disp_cnt.display_bg2,
    disp_cnt.display_bg3
  };

  for (int bg = 0; bg < 4; bg++) {
    // Skip rendering the background if it's not enabled.
    if (!display_bg[bg]) {
      continue;
    }

    BackgroundControl const& bg_control = *(BackgroundControl*)(bg_control_mem + bg * 2);
    uint8_t* base_bg_tile_ram = vram + bg_control.char_base_block * 0x4000;
    uint16_t* base_screen_block_ram = (uint16_t*)(vram + bg_control.screen_base_block * 0x800);
    bool is_rotation_scaling = disp_cnt.background_mode >= 2 || (disp_cnt.background_mode == 1 && bg == 2);

    uint32_t width_in_tiles = 0;
    uint32_t height_in_tiles = 0;
    gpu_get_bg_size_in_tiles(
      is_rotation_scaling,
      bg_control.screen_size,
      width_in_tiles,
      height_in_tiles
    );

    uint32_t width_in_pixels = width_in_tiles * TILE_SIZE;
    uint32_t height_in_pixels = height_in_tiles * TILE_SIZE;

    int32_t bg_offset_x = 0;
    int32_t bg_offset_y = 0;

    int16_t pa = 1 << 8;
    int16_t pb = 0;
    int16_t pc = 0;
    int16_t pd = 1 << 8;

    if (is_rotation_scaling && bg >= 2) {
      bg_offset_x = bg == 2 
        ? ram_read_word_from_io_registers_fast<REG_BG2_X_REF>(cpu.ram)
        : ram_read_word_from_io_registers_fast<REG_BG3_X_REF>(cpu.ram);
      bg_offset_y = bg == 2 
        ? ram_read_word_from_io_registers_fast<REG_BG2_Y_REF>(cpu.ram)
        : ram_read_word_from_io_registers_fast<REG_BG3_Y_REF>(cpu.ram);

      pa = bg == 2
        ? ram_read_half_word_from_io_registers_fast<REG_BG2_PARAM_A>(cpu.ram)
        : ram_read_half_word_from_io_registers_fast<REG_BG3_PARAM_A>(cpu.ram);
      pb = bg == 2
        ? ram_read_half_word_from_io_registers_fast<REG_BG2_PARAM_B>(cpu.ram)
        : ram_read_half_word_from_io_registers_fast<REG_BG3_PARAM_B>(cpu.ram);
      pc = bg == 2
        ? ram_read_half_word_from_io_registers_fast<REG_BG2_PARAM_C>(cpu.ram)
        : ram_read_half_word_from_io_registers_fast<REG_BG3_PARAM_C>(cpu.ram);
      pd = bg == 2
        ? ram_read_half_word_from_io_registers_fast<REG_BG2_PARAM_D>(cpu.ram)
        : ram_read_half_word_from_io_registers_fast<REG_BG3_PARAM_D>(cpu.ram);
    } else {
      uint8_t* bg_offset_x_mem = ram_read_memory_from_io_registers_fast<REG_BG0_X_OFFSET>(cpu.ram);
      uint8_t* bg_offset_y_mem = ram_read_memory_from_io_registers_fast<REG_BG0_Y_OFFSET>(cpu.ram);

      // TODO: Fix this, doesn't seem to be working...
      bg_offset_x = (int32_t)(*(int16_t*)(bg_offset_x_mem + bg * 4)) & 0x1FF;
      bg_offset_y = (int32_t)(*(int16_t*)(bg_offset_y_mem + bg * 4)) & 0x1FF;
    }

    for (int screen_x = 0; screen_x < FRAME_WIDTH; ++screen_x) {
      int texture_x = 0;
      int texture_y = 0;

      if (is_rotation_scaling) {
        // Formula for calculating the texture coordinates using the screen coordinates:
        // texture_pos = offset + dot(transform, screen_pos)
        
        // dot(transform, screen_pos)
        int transformed_x = pa * screen_x + pb * scanline;
        int transformed_y = pc * screen_x + pd * scanline;

        // offset + dot(transform, screen_pos)
        texture_x = (bg_offset_x + transformed_x) >> 8;
        texture_y = (bg_offset_y + transformed_y) >> 8;
      } else {
        texture_x = (screen_x + bg_offset_x) % width_in_pixels;
        texture_y = (scanline + bg_offset_y) % height_in_pixels;
      }

      // TODO: Handle wrap around.
      if (texture_x < 0 || texture_x >= width_in_pixels || texture_y < 0 || texture_y >= height_in_pixels) {
        continue;
      }

      // Get the tile coordinates.
      int tile_x = texture_x / TILE_SIZE;
      int tile_y = texture_y / TILE_SIZE;

      int pos_x_in_tile = texture_x % TILE_SIZE;
      int pos_y_in_tile = texture_y % TILE_SIZE;

      uint8_t tile_size_bytes = bg_control.is_256_color_mode ? TILE_8BPP_BYTES : TILE_4BPP_BYTES;

      if (is_rotation_scaling) {
        // 1 byte per entry. Always 256 color mode (8bpp).
        int screen_entry_idx = tile_y * width_in_tiles + tile_x;
        uint8_t tile_index = ((uint8_t*)base_screen_block_ram)[screen_entry_idx];
        uint8_t palette_number = *(base_bg_tile_ram + tile_index * tile_size_bytes + pos_y_in_tile * TILE_SIZE + pos_x_in_tile);
        
        // Zero palette number means transparent pixel for backgrounds.
        if (palette_number == 0) continue;

        uint16_t color = palette_ram[palette_number];
        color |= ENABLE_PIXEL;
        gpu.scanline_by_priority_and_pixel_source[screen_x][bg_control.priority][bg] = color;
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

        uint16_t screen_block_entry = ((uint16_t*)base_screen_block_ram)[screen_block_idx * 1024 + (tile_y % 32) * 32 + (tile_x % 32)];
        uint16_t tile_index = screen_block_entry & 0x3FF;
        uint8_t flip_mode = (screen_block_entry >> 10) & 0x3;
        bool horizontal_flip = flip_mode & 0x1;
        bool vertical_flip = flip_mode & 0x2;
        uint8_t palette_bank = (screen_block_entry >> 12) & 0xF;

        if (horizontal_flip) {
          pos_x_in_tile = TILE_SIZE - 1 - pos_x_in_tile;
        }
        if (vertical_flip) {
          pos_y_in_tile = TILE_SIZE - 1 - pos_y_in_tile;
        }

        uint8_t* tile_data = &base_bg_tile_ram[tile_index * tile_size_bytes];

        if (bg_control.is_256_color_mode) {
          // TODO: This code path is untested.
          uint8_t palette_index = tile_data[pos_y_in_tile * TILE_SIZE + pos_x_in_tile];
          uint16_t color = palette_ram[palette_index];

          // Skip transparent pixels.
          if (palette_index == 0) continue;

          color |= ENABLE_PIXEL;
          gpu.scanline_by_priority_and_pixel_source[screen_x][bg_control.priority][bg] = color;
        } else {
          uint8_t palette_indices = tile_data[pos_y_in_tile * HALF_TILE_SIZE + pos_x_in_tile / 2];
          uint8_t palette_index = pos_x_in_tile % 2 == 0
            ? palette_indices & 0xF 
            : (palette_indices >> 4) & 0xF;

          // Skip transparent pixels.
          if (palette_index == 0) continue;
          
          uint16_t color = palette_ram[palette_bank * 16 + palette_index];
          color |= ENABLE_PIXEL;
          gpu.scanline_by_priority_and_pixel_source[screen_x][bg_control.priority][bg] = color;
        }
      }
    }
  }
}

void gpu_render_obj_layer(CPU& cpu, GPU& gpu, uint8_t scanline) {
  uint16_t disp_cnt_data = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  DisplayControl const& disp_cnt = *(DisplayControl*)&disp_cnt_data;

  uint8_t* vram = cpu.ram.video_ram;
  uint16_t* oam = (uint16_t*)cpu.ram.object_attribute_memory;
  uint16_t* sprite_palette_ram = (uint16_t*)(cpu.ram.palette_ram + 0x200);
  uint8_t* base_sprite_tile_ram = vram + 0x10000;

  // Sprite RAM changes depending on the background mode.
  if (disp_cnt.background_mode >= 3) {
    base_sprite_tile_ram += 0x4000;
  }

  for (int i = 127; i >= 0; i--) {
    uint16_t attr0 = oam[i * 4];

    bool rotation_scaling = attr0 & (1 << 8);
    bool disabled_or_double_size = attr0 & (1 << 9);

    // OBJ is disabled, skip it.
    if (!rotation_scaling && disabled_or_double_size) {
      continue;
    }

    uint16_t attr1 = oam[i * 4 + 1];
    uint16_t attr2 = oam[i * 4 + 2];

    int16_t x_coord = attr1 & 0x1FF;
    int16_t y_coord = attr0 & 0xFF;

    bool is_256_color_mode = attr0 & (1 << 13);
    uint8_t shape_enum = (attr0 & (3 << 14)) >> 14;
    uint8_t size_enum = (attr1 & (3 << 14)) >> 14;

    bool horizontal_flip = attr1 & (1 << 12);
    bool vertical_flip = attr1 & (1 << 13);

    uint8_t width = 0;
    uint8_t height = 0;
    gpu_get_obj_size(shape_enum, size_enum, width, height);

    int bbox_width = width;
    int bbox_height = height;
    if (disabled_or_double_size) {
      bbox_width *= 2;
      bbox_height *= 2;
    }

    // Handle position wrap-around.
    if (y_coord > 160) {
      // Make sure y_coord is in range [-128, 127]
      y_coord -= 256;
    }

    if (disabled_or_double_size && y_coord + bbox_height > 256) {
      // Edge case: double size sprites that are at the edge of the screen.
      y_coord -= 256;
    }
  
    // Check if the sprite is on this scanline.
    if (scanline < y_coord || scanline >= y_coord + bbox_height) {
      continue;
    }

    // Make sure x_coord is in range [-256, 255]
    if (x_coord > 255) {
      x_coord -= 512;
    }

    uint16_t tile_base = attr2 & 0x3FF;
    uint8_t palette_number = attr2 >> 12; // 16 colors mode only

    if (is_256_color_mode) {
      // First bit of tile number is ignored when in 256 color mode.
      tile_base >>= 1;
    }

    int16_t pa = 1 << 8;
    int16_t pb = 0;
    int16_t pc = 0;
    int16_t pd = 1 << 8;
    if (rotation_scaling) {
      gpu_get_obj_affine_params(cpu, attr1, pa, pb, pc, pd);
    }

    uint8_t width_in_tiles = width / TILE_SIZE;
    uint8_t height_in_tiles = height / TILE_SIZE;
    uint8_t number_of_tiles = width_in_tiles * height_in_tiles;
    uint8_t tile_size_bytes = is_256_color_mode ? TILE_8BPP_BYTES : TILE_4BPP_BYTES;

    uint8_t y_in_draw_area = scanline - y_coord;
    uint8_t half_width = bbox_width / 2;
    uint8_t half_height = bbox_height / 2;

    uint8_t center_x_texture_space = width / 2;
    uint8_t center_y_texture_space = height / 2;

    int16_t center_x_screen_space = x_coord + half_width;
    int16_t center_y_screen_space = y_coord + half_height;

    OBJMode obj_mode = (OBJMode)((attr0 >> 10) & 0x3);
    uint8_t priority = (attr2 >> 10) & 0x3;

    int iy = y_in_draw_area - half_height;
    for (int ix = -half_width; ix < half_width; ix++) {
      int texture_x = 0;
      int texture_y = 0;

      if (rotation_scaling) {
        texture_x = (pa * ix + pb * iy) >> 8;
        texture_y = (pc * ix + pd * iy) >> 8;
      } else {
        texture_x = ix;
        texture_y = iy;
      }

      texture_x += center_x_texture_space;
      texture_y += center_y_texture_space;

      int x = center_x_screen_space + ix;
      int y = center_y_screen_space + iy;

      if (y != scanline) {
        continue;
      }

      if (x < 0 || x >= FRAME_WIDTH || y < 0 || y >= FRAME_HEIGHT) {
        continue;
      }

      if (texture_x < 0 || texture_x >= width || texture_y < 0 || texture_y >= height) {
        continue;
      }

      if (!rotation_scaling) {
        // TODO: These branches are untested.
        if (horizontal_flip) {
          texture_x = width - texture_x - 1;
        }

        if (vertical_flip) {
          texture_y = height - texture_y - 1;
        }
      }

      uint8_t col_idx = texture_x / TILE_SIZE;
      uint8_t row_idx = texture_y / TILE_SIZE;

      uint16_t tile_idx = tile_base + row_idx * width_in_tiles + col_idx;
      if (!disp_cnt.one_dimensional_mapping) {
        if (is_256_color_mode) {
          tile_idx = tile_base + row_idx * 16 + col_idx;
        } else {
          tile_idx = tile_base + row_idx * 32 + col_idx;
        }
      }

      uint8_t tile_x = col_idx * TILE_SIZE;
      uint8_t tile_y = row_idx * TILE_SIZE;
      uint8_t texture_x_in_tile = texture_x - tile_x;
      uint8_t texture_y_in_tile = texture_y - tile_y;
      uint8_t* current_tile = &base_sprite_tile_ram[tile_idx * tile_size_bytes];

      if (is_256_color_mode) {
        uint8_t palette_idx = current_tile[texture_y_in_tile * TILE_SIZE + texture_x_in_tile];
        uint16_t color = sprite_palette_ram[palette_idx];

        // Skip transparent pixels.
        if (palette_idx == 0) continue;

        if (obj_mode != OBJ_MODE_WINDOW) {
          color |= ENABLE_PIXEL;
          gpu.scanline_by_priority_and_pixel_source[x][priority][PIXEL_SOURCE_OBJ] = color;
        }
      } else {
        uint8_t palette_indices = current_tile[texture_y_in_tile * HALF_TILE_SIZE + texture_x_in_tile / 2];
        uint8_t palette_idx = texture_x_in_tile % 2 == 0
          ? palette_indices & 0xF
          : (palette_indices >> 4) & 0xF;
        uint16_t color = sprite_palette_ram[palette_number * 16 + palette_idx];
       
        // Skip transparent pixels.
        if (palette_idx == 0) continue;
        
        if (obj_mode != OBJ_MODE_WINDOW) {
          color |= ENABLE_PIXEL;
          gpu.scanline_by_priority_and_pixel_source[x][priority][PIXEL_SOURCE_OBJ] = color;
        }
      }

      if (obj_mode == OBJ_MODE_SEMI_TRANSPARENT) {
        gpu.scanline_semi_transparent_buffer[x] = true;
      } else if (obj_mode == OBJ_MODE_WINDOW) {
        gpu.scanline_obj_window_buffer[x] = true;
      }
    }
  }
}

void gpu_render_scanline(CPU& cpu, GPU& gpu, uint8_t scanline) {
  // Reset layer buffers.
  gpu_clear_scanline_buffers(gpu);

  // Backdrop Layer.
  uint16_t backdrop_color = gpu_get_backdrop_color(cpu);
  for (int i = 0; i < FRAME_WIDTH; i++) {
    gpu.scanline_buffer[i] = backdrop_color;
  }

  // BG Layers.
  gpu_render_bg_layer(cpu, gpu, scanline);

  // OBJ Layer.
  gpu_render_obj_layer(cpu, gpu, scanline);

  // Apply Window Effects
  gpu_apply_window_effects(cpu, gpu);

  // Apply Special Effects
  gpu_apply_special_effects(cpu, gpu);

  // Apply Window to Special Effects
  gpu_apply_window_to_special_effects(cpu, gpu);

  // Composite the various priority buffers to a final scanline.
  gpu_resolve_scanline_buffer(cpu, gpu);

  // Copy the final scanline buffer to the frame buffer.
  uint32_t frame_buffer_offset = scanline * FRAME_WIDTH;
  memcpy(&gpu.frame_buffer[frame_buffer_offset], gpu.scanline_buffer, FRAME_WIDTH * sizeof(uint16_t));
}

void gpu_complete_scanline(CPU& cpu, GPU& gpu) {
  uint8_t scanline = ram_read_byte_from_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram);
  uint16_t lcd_status = ram_read_half_word_from_io_registers_fast<REG_LCD_STATUS>(cpu.ram);

  // Begin VCount match
  uint8_t vertical_count_setting = lcd_status >> 8;
  if (scanline == vertical_count_setting) {
    lcd_status |= REG_LCD_STATUS_VCOUNT_MATCH_FLAG;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);

    // Request VCount interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_VCOUNT_MATCH_INTERRUPT_ENABLE) {
      uint16_t interrupt_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
      interrupt_flags |= REG_LCD_STATUS_VCOUNT_MATCH_FLAG;

      // NOTE: The following write must skip write hooks, since this register is clear-on-write.
      ram_write_half_word_to_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram, interrupt_flags);
    }
  } else if (lcd_status & REG_LCD_STATUS_VCOUNT_MATCH_FLAG) {
    // End VCount match
    lcd_status &= ~REG_LCD_STATUS_VCOUNT_MATCH_FLAG;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);
  }

  // Begin VBlank
  if (scanline == 160) {
    // std::cout << "VBlank" << std::endl;
    lcd_status |= 0x1;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);

    // Request VBlank interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_VBLANK_INTERRUPT_ENABLE) {
      // std::cout << "VBlank interrupt" << std::endl;
      uint16_t interrupt_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
      interrupt_flags |= 0x1;
      // NOTE: The following write must skip write hooks, since this register is clear-on-write.
      ram_write_half_word_to_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram, interrupt_flags);
    }
  }

  if (scanline < 160) {
    // Render scanline if not in VBlank.
    gpu_render_scanline(cpu, gpu, scanline);
  }

  // End VBlank
  if (scanline == 226) {
    uint16_t lcd_status = ram_read_half_word_from_io_registers_fast<REG_LCD_STATUS>(cpu.ram);
    lcd_status &= ~0x1;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);
  }

  // Increment scanline
  scanline++;
  if (scanline == 228) {
    scanline = 0;
  }
  ram_write_byte_to_io_registers_fast<REG_VERTICAL_COUNT>(cpu.ram, scanline);
}

void gpu_cycle(CPU& cpu, GPU& gpu) {
  if (cpu.cycle_count % 1232 == 0) {
    gpu_complete_scanline(cpu, gpu);
  }

  // Calculate how many cycles we are into the current scanline
  uint32_t cycles_into_scanline = 1232 - cpu.cycle_count % 1232;
  if (cycles_into_scanline == 1232 - 272) {
    // Begin HBlank
    uint16_t lcd_status = ram_read_half_word_from_io_registers_fast<REG_LCD_STATUS>(cpu.ram);
    lcd_status |= 1 << 1;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);

    // Request HBlank interrupt (if it is enabled)
    if (lcd_status & REG_LCD_STATUS_HBLANK_INTERRUPT_ENABLE) {
      uint16_t interrupt_flags = ram_read_half_word_from_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram);
      interrupt_flags |= REG_LCD_STATUS_HBLANK_FLAG;
      // NOTE: The following write MUST skip the write hooks, since this register is clear-on-write.
      ram_write_half_word_to_io_registers_fast<REG_INTERRUPT_REQUEST_FLAGS>(cpu.ram, interrupt_flags);
    }
  } else if (cycles_into_scanline == 0) {
    // End HBlank
    uint16_t lcd_status = ram_read_half_word_from_io_registers_fast<REG_LCD_STATUS>(cpu.ram);
    lcd_status &= ~REG_LCD_STATUS_HBLANK_FLAG;
    ram_write_half_word_to_io_registers_fast<REG_LCD_STATUS>(cpu.ram, lcd_status);
  }
}
