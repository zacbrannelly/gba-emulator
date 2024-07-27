#include "gpu.h"
#include "debug.h"
#include <cstring>

static constexpr uint16_t REG_LCD_STATUS_HBLANK_FLAG = 1 << 1;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_FLAG = 1 << 2;
static constexpr uint16_t REG_LCD_STATUS_VBLANK_INTERRUPT_ENABLE = 1 << 3;
static constexpr uint16_t REG_LCD_STATUS_HBLANK_INTERRUPT_ENABLE = 1 << 4;
static constexpr uint16_t REG_LCD_STATUS_VCOUNT_MATCH_INTERRUPT_ENABLE = 1 << 5;

static constexpr uint32_t TILE_SIZE = 8;
static constexpr uint32_t HALF_TILE_SIZE = 4;
static constexpr uint32_t TILE_4BPP_BYTES = 32;
static constexpr uint32_t TILE_8BPP_BYTES = 64;
static constexpr uint16_t ENABLE_PIXEL = 1 << 15;

void gpu_init(CPU& cpu, GPU& gpu) {
  // Initialize the frame buffer to white.
  for (uint32_t i = 0; i < FRAME_BUFFER_SIZE; i++) {
    gpu.frame_buffer[i] = 0xFFFF;
  }

  // TODO: Initialize the GPU registers.
}

static constexpr int TARGET_LAYER_BG0 = 0;
static constexpr int TARGET_LAYER_BG1 = 1;
static constexpr int TARGET_LAYER_BG2 = 2;
static constexpr int TARGET_LAYER_BG3 = 3;
static constexpr int TARGET_LAYER_OBJ = 4;
static constexpr int TARGET_LAYER_BACKDROP = 5;

inline void gpu_apply_special_effects(CPU& cpu, GPU& gpu) {
  uint16_t special_effects_control = ram_read_half_word_from_io_registers_fast<REG_BLDCNT>(cpu.ram);

  uint8_t special_effects_mode = (special_effects_control >> 6) & 0x3;
  bool target_1[6] = {
    (special_effects_control & 0x1) > 0,
    (special_effects_control & 0x2) > 0,
    (special_effects_control & 0x4) > 0,
    (special_effects_control & 0x8) > 0,
    (special_effects_control & 0x10) > 0,
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
        uint8_t target_1_priority = 0;
        uint8_t target_1_idx = 0;
        uint16_t target_1_color = 0;

        // Get top most pixel and it's source.
        for (int j = 0; j < 4; j++) {
          uint16_t color = gpu.scanline_priority_buffers[j][i];
          if (color > 0) {
            target_1_priority = j;
            target_1_color = color;
            target_1_idx = (uint8_t)gpu.scanline_priority_pixel_sources[j][i];
            break;
          }
        }

        // Check if the target is enabled.
        if (!target_1[target_1_idx - 1]) continue;

        uint8_t target_2_priority = 0;
        uint8_t target_2_idx = 0;
        uint16_t target_2_color = 0;

        // Get the next top most pixel and it's source.
        for (int j = target_1_priority + 1; j < 4; j++) {
          uint16_t color = gpu.scanline_priority_buffers[j][i];
          if (color > 0) {
            target_2_priority = j;
            target_2_color = color;
            target_2_idx = (uint8_t)gpu.scanline_priority_pixel_sources[j][i];
            break;
          }
        }

        // Check if the target is enabled.
        if (!target_2[target_2_idx - 1]) continue;

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
          gpu.final_scanline_buffer[i] = blended_color | ENABLE_PIXEL;
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
        uint8_t target_1_idx = 0;
        uint16_t target_1_color = 0;

        // Get top most pixel and it's source.
        for (int j = 0; j < 4; j++) {
          uint16_t color = gpu.scanline_priority_buffers[j][i];
          if (color > 0) {
            target_1_color = color;
            target_1_idx = (uint8_t)gpu.scanline_priority_pixel_sources[j][i];
            break;
          }
        }

        // Check if the target is enabled.
        if (!target_1[target_1_idx - 1]) continue;

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
          gpu.final_scanline_buffer[i] = blended_color | ENABLE_PIXEL;
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

inline void gpu_resolve_final_scanline_buffer(CPU& cpu, GPU& gpu) {
  for (int i = 0; i < FRAME_WIDTH; i++) {
    for (int j = 3; j >= 0; j--) {
      uint16_t color = gpu.scanline_priority_buffers[j][i];
      if (color > 0) {
        gpu.final_scanline_buffer[i] = color;
      }
    }
  }
}

inline void gpu_clear_scanline_buffers(GPU& gpu) {
  // Reset layer buffers.
  memset(gpu.final_scanline_buffer, 0, FRAME_WIDTH * sizeof(uint16_t));
  for (int i = 0; i < 4; i++) {
    memset(gpu.scanline_priority_buffers[i], 0, FRAME_WIDTH * sizeof(uint16_t));
    for (int j = 0; j < FRAME_WIDTH; j++) {
      gpu.scanline_priority_pixel_sources[i][j] = PIXEL_SOURCE_NONE;
    }
  }
}

void gpu_render_scanline(CPU& cpu, GPU& gpu, uint8_t scanline) {
  // Reset layer buffers.
  gpu_clear_scanline_buffers(gpu);

  uint16_t disp_cnt = ram_read_half_word_from_io_registers_fast<REG_LCD_CONTROL>(cpu.ram);
  uint8_t* vram = cpu.ram.video_ram;
  uint16_t* bg_palette_ram = (uint16_t*)(cpu.ram.palette_ram);

  uint16_t backdrop_color = bg_palette_ram[0];
  if (backdrop_color > 0) {
    backdrop_color |= ENABLE_PIXEL;
  }

  // =================================================================================================
  // Backdrop Layer
  // =================================================================================================
  for (uint32_t i = 0; i < FRAME_WIDTH; i++) {
    gpu.scanline_priority_buffers[3][i] = backdrop_color;
    gpu.scanline_priority_pixel_sources[3][i] = PIXEL_SOURCE_BACKDROP;
  }

  // =================================================================================================
  // BG Layers
  // =================================================================================================

  // TODO: Implement the logic for each BG layer.

  // =================================================================================================
  // OBJ Layer
  // =================================================================================================
  bool is_one_dimensional = disp_cnt & (1 << 6);

  uint16_t* oam = (uint16_t*)cpu.ram.object_attribute_memory;
  uint16_t* sprite_palette_ram = (uint16_t*)(cpu.ram.palette_ram + 0x200);

  // TODO: This changes depending on the background mode.
  uint8_t* base_sprite_tile_ram = vram + 0x10000;

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
    if (x_coord > 255) {
      x_coord -= 256;
    }
    if (disabled_or_double_size) {
      if (y_coord + bbox_height >= 256) {
        y_coord -= 256;
      }
    } else {
      if (y_coord >= 160) {
        y_coord -= 256;
      }
    }

    // Check if the sprite is on this scanline.
    if (scanline < y_coord || scanline >= y_coord + bbox_height) {
      continue;
    }

    uint16_t tile_base = attr2 & 0x3FF;
    uint8_t palette_number = attr2 >> 12; // 16 colors mode only

    if (!is_256_color_mode) {
      sprite_palette_ram += palette_number * 16;
    } else {
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

    uint8_t center_x_screen_space = x_coord + half_width;
    uint8_t center_y_screen_space = y_coord + half_height;

    uint8_t priority = (attr2 >> 10) & 0x3;

    int iy = y_in_draw_area - half_height;
    for (int ix = -half_width; ix < half_width; ix++) {
      int texture_x = (pa * ix + pb * iy) >> 8;
      int texture_y = (pc * ix + pd * iy) >> 8;

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
      if (!is_one_dimensional) {
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

        if (color > 0) {
          color |= ENABLE_PIXEL;
        } else {
          // Skip transparent pixels.
          continue;
        }

        gpu.scanline_priority_buffers[priority][x] = color;
        gpu.scanline_priority_pixel_sources[priority][x] = PIXEL_SOURCE_OBJ;
      } else {
        // TODO: This code path is untested.
        uint8_t palette_indices = current_tile[texture_y_in_tile * HALF_TILE_SIZE + texture_x_in_tile / 2];
        uint16_t color_left = sprite_palette_ram[palette_indices & 0xF];
        uint16_t color_right = sprite_palette_ram[palette_indices >> 4];

        if (color_left > 0) {
          color_left |= ENABLE_PIXEL;
          gpu.scanline_priority_buffers[priority][x] = color_left;
          gpu.scanline_priority_pixel_sources[priority][x] = PIXEL_SOURCE_OBJ;
        }

        if (color_right > 0) {
          color_right |= ENABLE_PIXEL;
          gpu.scanline_priority_buffers[priority][x + 1] = color_right;
          gpu.scanline_priority_pixel_sources[priority][x + 1] = PIXEL_SOURCE_OBJ;
        }
      }
    }
  }

  // Composite the various priority buffers to a final scanline.
  gpu_resolve_final_scanline_buffer(cpu, gpu);

  // Apply Special Effects
  gpu_apply_special_effects(cpu, gpu);

  // Copy the final scanline buffer to the frame buffer.
  uint32_t frame_buffer_offset = scanline * FRAME_WIDTH;
  memcpy(&gpu.frame_buffer[frame_buffer_offset], gpu.final_scanline_buffer, FRAME_WIDTH * sizeof(uint16_t));
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
      std::cout << "HBlank interrupt" << std::endl;
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
