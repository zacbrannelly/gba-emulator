#!/bin/bash

docker run \
  --rm \
  -v $(pwd):/root \
  -w /root devkitpro/devkitarm:latest \
  ./scripts/devkit_compile_assembly.sh $1
