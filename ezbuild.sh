#!/bin/bash

echo "开始构建"

rm -rf build
mkdir build
cd build
cmake ..
make

dd if=/dev/zero of=diskimg bs=1024 count=10000
./init_disk
mkdir mountdir

echo "构建完成"
