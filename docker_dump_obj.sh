#!/bin/bash

docker run \
  --rm \
  -v $(pwd):/root \
  -w /root devkitpro/devkitarm:latest \
  ./scripts/devkit_dump_object.sh $1
