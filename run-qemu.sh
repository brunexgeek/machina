#!/bin/bash

SERIAL="-serial stdio"

if [ "$1" == "--debug" ]; then
    EXTRA_ARGS="-S -s"
    shift 1
fi

if [ "$1" == "--intr" ]; then
    EXTRA_ARGS="$EXTRA_ARGS -d int"
    shift 1
fi

if [ "$1" == "--mon" ]; then
    EXTRA_ARGS="$EXTRA_ARGS -monitor stdio"
    SERIAL=""
    shift 1
fi

#/media/dados/github/qemu/build/qemu-system-arm -kernel "$1" -M raspi2 -serial stdio
/media/dados/github/qemu/build/qemu-system-aarch64 -icount 6 $EXTRA_ARGS -kernel "$1" -M raspi3 $SERIAL
#/media/dados/github/qemu/build/qemu-system-aarch64 -kernel "$1" -M raspi3 -serial null -serial stdio
