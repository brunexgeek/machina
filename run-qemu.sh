#!/bin/bash

qemu-system-arm -kernel "$1" -M raspi2 -serial stdio -icount shift=16
