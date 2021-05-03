#!/bin/bash

set -x
set -e
set -u

echo "[+] Installing LLVM"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then

    wget https://apt.llvm.org/llvm.sh
    chmod +x ./llvm.sh
    sudo ./llvm.sh 12
    rm ./llvm.sh

elif [[ "$OSTYPE" == "darwin"* ]]; then
    brew install llvm # llvm@12
fi