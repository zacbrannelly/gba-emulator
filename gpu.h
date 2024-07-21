#pragma once

#include "cpu.h"

static constexpr uint32_t FRAME_WIDTH = 240;
static constexpr uint32_t FRAME_HEIGHT = 160;
static constexpr uint32_t FRAME_BUFFER_SIZE = FRAME_WIDTH * FRAME_HEIGHT;
static constexpr uint32_t FRAME_BUFFER_SIZE_BYTES = FRAME_BUFFER_SIZE * sizeof(uint16_t);
static constexpr uint32_t FRAME_BUFFER_PITCH = FRAME_WIDTH;

struct GPU {
  uint16_t frameBuffer[FRAME_BUFFER_SIZE];
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
