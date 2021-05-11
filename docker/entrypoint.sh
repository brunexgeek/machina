#!/bin/bash

rm -rf /build/*

mkdir -p /build && \
cd /build && \
cmake -DCMAKE_TOOLCHAIN_FILE=/machina/toolchain.cmake /machina && \
make image