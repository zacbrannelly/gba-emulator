#pragma once

#include "cpu.h"

static constexpr uint32_t FRAME_WIDTH = 240;
static constexpr uint32_t FRAME_HEIGHT = 160;
static constexpr uint32_t FRAME_BUFFER_SIZE = FRAME_WIDTH * FRAME_HEIGHT;
static constexpr uint32_t FRAME_BUFFER_SIZE_BYTES = FRAME_BUFFER_SIZE * sizeof(uint16_t);
static constexpr uint32_t FRAME_BUFFER_PITCH = FRAME_WIDTH;

static constexpr uint32_t TILE_SIZE = 8;
static constexpr uint32_t HALF_TILE_SIZE = 4;
static constexpr uint32_t TILE_4BPP_BYTES = 32;
static constexpr uint32_t TILE_8BPP_BYTES = 64;
static constexpr uint16_t ENABLE_PIXEL = 1 << 15;

enum PixelSource {
  PIXEL_SOURCE_BG0 = 0,
  PIXEL_SOURCE_BG1 = 1,
  PIXEL_SOURCE_BG2 = 2,
  PIXEL_SOURCE_BG3 = 3,
  PIXEL_SOURCE_OBJ = 4,
  PIXEL_SOURCE_BACKDROP = 5,
};

enum OBJMode {
  OBJ_MODE_REGULAR_SPRITE = 0,
  OBJ_MODE_SEMI_TRANSPARENT = 1,
  OBJ_MODE_WINDOW = 2,
  OBJ_MODE_PROHIBITED = 3
};

struct DisplayControl {
  uint8_t background_mode : 3;
  uint8_t reserved : 1;
  uint8_t display_frame_select : 1;
  bool hblank_interval_free : 1;
  bool one_dimensional_mapping : 1;
  bool force_blank : 1;
  bool display_bg0 : 1;
  bool display_bg1 : 1;
  bool display_bg2 : 1;
  bool display_bg3 : 1;
  bool display_obj : 1;
  bool display_window0 : 1;
  bool display_window1 : 1;
  bool display_obj_window : 1;
};

struct BackgroundControl {
  uint8_t priority : 2;
  // 0 - 3, in units of 16kb, where the tile data is stored in VRAM.
  uint8_t char_base_block : 2;
  uint8_t reserved : 2;
  bool mosaic : 1;
  bool is_256_color_mode : 1;
  uint8_t screen_base_block : 5;
  bool display_area_overflow : 1;
  uint8_t screen_size : 2;
};

struct GPU {
  // 4 priority levels, 4 possible pixel sources (BACKDROP is not used here).
  uint16_t scanline_by_priority_and_pixel_source[FRAME_WIDTH][4][5];
  uint16_t scanline_special_effects_buffer[FRAME_WIDTH];

  // Semi-Transparent Buffer.
  bool scanline_semi_transparent_buffer[FRAME_WIDTH];

  // Mask sourced from OBJ Windows.
  bool scanline_obj_window_buffer[FRAME_WIDTH];

  // Final scanline color buffer.
  uint16_t scanline_buffer[FRAME_WIDTH];

  // Full Frame buffer.
  uint16_t frame_buffer[FRAME_BUFFER_SIZE];
};

void gpu_init(CPU& cpu, GPU& gpu);
void gpu_cycle(CPU& cpu, GPU& gpu);

inline void gpu_get_obj_affine_params(CPU& cpu, uint16_t attr1, int16_t& pa, int16_t& pb, int16_t& pc, int16_t& pd) {
  // Rotation / Scaling parameters
  // 1st Group - PA=07000006, PB=0700000E, PC=07000016, PD=0700001E
  // 2nd Group - PA=07000026, PB=0700002E, PC=07000036, PD=0700003E
  // 3rd Group - PA=07000046, PB=0700004E, PC=07000056, PD=0700005E
  // 4th Group - PA=07000066, PB=0700006E, PC=07000076, PD=0700007E
  // 5th Group - PA=07000086, PB=0700008E, PC=07000096, PD=0700009E
  // 6th Group - PA=070000A6, PB=070000AE, PC=070000B6, PD=070000BE
  // 7th Group - PA=070000C6, PB=070000CE, PC=070000D6, PD=070000DE
  // etc.
  uint8_t matrix_index = (attr1 & (0xF << 9)) >> 9;
  int16_t* rotation_scaling_params = (int16_t*)(cpu.ram.object_attribute_memory + 0x6 + matrix_index * 0x20);
  pa = rotation_scaling_params[0];
  pb = rotation_scaling_params[4];
  pc = rotation_scaling_params[8];
  pd = rotation_scaling_params[12];
}

inline void gpu_get_bg_size_in_tiles(
  bool is_rotation_scaling,
  uint8_t screen_size,
  uint32_t& width,
  uint32_t& height
) {
  switch (screen_size) {
    case 0:
      width = is_rotation_scaling ? 16 : 32;
      height = is_rotation_scaling ? 16 : 32;
      break;
    case 1:
      width = is_rotation_scaling ? 32 : 64;
      height = is_rotation_scaling ? 32 : 32;
      break;
    case 2:
      width = is_rotation_scaling ? 64 : 32;
      height = is_rotation_scaling ? 64 : 64;
      break;
    case 3:
      width = is_rotation_scaling ? 128 : 64;
      height = is_rotation_scaling ? 128 : 64;
      break;
  }
}

inline void gpu_get_obj_size(uint8_t shape_enum, uint8_t size_enum, uint8_t& width, uint8_t& height) {
  switch (shape_enum) {
    case 0: {
      // Square
      switch (size_enum) {
        case 0:
          width = 8;
          height = 8;
          break;
        case 1:
          width = 16;
          height = 16;
          break;
        case 2:
          width = 32;
          height = 32;
          break;
        case 3:
          width = 64;
          height = 64;
          break;
      }
      break;
    }
    case 1: {
      // Horizontal
      switch (size_enum) {
        case 0:
          width = 16;
          height = 8;
          break;
        case 1:
          width = 32;
          height = 8;
          break;
        case 2:
          width = 32;
          height = 16;
          break;
        case 3:
          width = 64;
          height = 32;
          break;
      }
      break;
    }
    case 2: {
      // Vertical
      switch (size_enum) {
        case 0:
          width = 8;
          height = 16;
          break;
        case 1:
          width = 8;
          height = 32;
          break;
        case 2:
          width = 16;
          height = 32;
          break;
        case 3:
          width = 32;
          height = 64;
          break;
      }
      break;
    }
  }
}
