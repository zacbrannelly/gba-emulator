#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Create build directory if it doesn't exist
mkdir -p build

# Configure CMake with tests enabled
cmake -DBUILD_TESTS=ON -DCI_RUNNER=ON -S . -B build

# Build the test runner
cmake --build ./build --target test_runner -j 12

# Run the tests
cmake --build ./build --target run_tests
