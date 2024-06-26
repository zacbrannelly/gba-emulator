#!/bin/bash

# Stop execution if a command fails
set -e

# Get all *.cpp files in the test directory using glob
CPP_FILES=$(find tests -name "*.cpp")

# Make the bin directory if it doesn't exist
mkdir -p ./tests/bin

# Build the test runner
echo "Building test runner..."
g++ -o ./tests/bin/test_runner \
  $CPP_FILES \
  ./3rdparty/catch_amalgamated.cpp \
  cpu.cpp \
  ram.cpp \
  dma.cpp \
  -I./3rdparty \
  -I./ \
  -std=c++20

# Run the test runner
echo "Running test runner..."
./tests/bin/test_runner
