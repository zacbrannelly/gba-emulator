#!/bin/bash

PATH=$PATH:/opt/devkitpro/devkitARM/bin

arm-none-eabi-objdump -d $1
