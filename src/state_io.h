#pragma once

#include "cpu.h"

void save_state(CPU& cpu, std::string const& state_file_path);
void load_state(CPU& cpu, std::string const& state_file_path);
