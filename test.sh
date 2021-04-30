#!/bin/bash

set -e
set -u
set -x

export PATH="$LLVM_DIR/bin:$PATH"

clang -O1 -emit-llvm -c inputs/input1.c -o inputs/input1.bc

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    opt -load=build/libRAP.so -rap inputs/input1.bc -o inputs/input1.rap.bc
elif [[ "$OSTYPE" == "darwin"* ]]; then
    opt -load=build/libRAP.dylib -rap inputs/input1.bc -o inputs/input1.rap.bc
fi

clang inputs/input1.rap.bc -o input1