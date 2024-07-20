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

inline void get_obj_size(uint8_t shape_enum, uint8_t size_enum, uint8_t& width, uint8_t& height) {
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
