#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Set build directory
BUILD_DIR="build"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Configure CMake without tests
cmake -DBUILD_TESTS=OFF -S . -B $BUILD_DIR

# Build the emulator
cmake --build $BUILD_DIR --target emulator -j 12

# Run the emulator
$BUILD_DIR/emulator
