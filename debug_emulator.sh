#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Set build directory
BUILD_DIR="build_debug"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Configure CMake in Debug mode without tests
cmake -S . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF

# Build the emulator with debug symbols
cmake --build $BUILD_DIR --target emulator -j 12

# Run the emulator with GDB
lldb $BUILD_DIR/emulator
