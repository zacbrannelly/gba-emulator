#!/bin/sh

PATH=$PATH:/opt/devkitpro/devkitARM/bin

arm-none-eabi-as -mcpu=arm7tdmi -mthumb-interwork -o $1.o $1
arm-none-eabi-objcopy -O binary $1.o $1.bin