#!/bin/bash

# Stop execution if a command fails
set -e

# Make the bin directory if it doesn't exist
mkdir -p ./bin

# Build the test runner
echo "Building..."
g++ -O3 -o ./bin/emulator \
  cpu.cpp \
  ram.cpp \
  emulator.cpp \
  dma.cpp \
  timer.cpp \
  gpu.cpp \
  -I./3rdparty \
  -I./ \
  -std=c++20 \

# Run the emulator
echo "Running..."
./bin/emulator
