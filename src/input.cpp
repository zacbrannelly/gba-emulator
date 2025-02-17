#include "input.h"
#include "3rdparty/zengine/ZEngine-Core/Input/InputManager.h"

#define DEFINE_GBA_BUTTON(name, value) \
  static constexpr uint16_t GBA_BUTTON_##name = value; \
  static constexpr uint16_t GBA_BUTTON_##name##_DOWN = ~value; \
  static constexpr uint16_t GBA_BUTTON_##name##_UP = value;

#define MAP_KEY_TO_GBA_BUTTON(key, button) \
  if (inputManager->GetButtonDown(key)) { \
    key_status &= GBA_BUTTON_##button##_DOWN; \
  } else { \
    key_status |= GBA_BUTTON_##button##_UP; \
  }

// Define constants for the GBA buttons.
DEFINE_GBA_BUTTON(A, 1)
DEFINE_GBA_BUTTON(B, (1 << 1))
DEFINE_GBA_BUTTON(SELECT, (1 << 2))
DEFINE_GBA_BUTTON(START, (1 << 3))
DEFINE_GBA_BUTTON(RIGHT, (1 << 4))
DEFINE_GBA_BUTTON(LEFT, (1 << 5))
DEFINE_GBA_BUTTON(UP, (1 << 6))
DEFINE_GBA_BUTTON(DOWN, (1 << 7))

void input_handle_key_detection(CPU& cpu, ZEngine::InputManager* inputManager) {
  // Read the previous key status.
  uint16_t key_status = ram_read_half_word_from_io_registers_fast<REG_KEY_STATUS>(cpu.ram);

  // Map the keyboard to the GBA buttons.
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_A,     A)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_B,     B)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_SPACE, START)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_ENTER, SELECT)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_RIGHT, RIGHT)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_LEFT,  LEFT)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_UP,    UP)
  MAP_KEY_TO_GBA_BUTTON(ZEngine::BUTTON_KEY_DOWN,  DOWN)

  // Write the new key status.
  ram_write_half_word_to_io_registers_fast<REG_KEY_STATUS>(cpu.ram, key_status & 0x3FF);
}
