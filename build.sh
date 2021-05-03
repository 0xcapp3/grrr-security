#!/bin/bash

set -x
set -e
set -u

echo "[+] Building src"

# export LLVM_DIR="/usr/local/Cellar/llvm/12.0.0"
export PATH="$LLVM_DIR/bin:$PATH"

rm -r build || true
mkdir -p build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ../RAP
make
