#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Set build directory
BUILD_DIR="build_debug"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Configure CMake in Debug mode with tests enabled
cmake -S . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build the test runner with debug symbols
cmake --build $BUILD_DIR --target test_runner -j 12

# Run the tests with a debugger
lldb $BUILD_DIR/test_runner
