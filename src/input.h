#pragma once

#include "cpu.h"

namespace ZEngine
{
  class InputManager;
}

void input_handle_key_detection(CPU& cpu, ZEngine::InputManager* inputManager);
