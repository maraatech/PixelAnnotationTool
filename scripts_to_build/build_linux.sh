#!/bin/bash

if [ -d "build" ]; then
	rm -r build
fi
mkdir build
cd build
cmake -DQT5_DIR=/home/anyone/Qt5.9.1/5.9.1/gcc_64/lib/cmake -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-8.0 -G"Unix Makefiles" ../..
make
cd ..
if [ -d "../build" ]; then
	rm -r ../build
fi
mv build ../build
