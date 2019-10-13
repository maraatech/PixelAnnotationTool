#!/bin/bash
#
## IMPORTANT:
# Before running this script please fill in the required fields in CMakeLists.txt

if [ -d "build" ]; then
	rm -r build
fi
mkdir build
cd build
cmake -G"Unix Makefiles" ../..
make
cd ..
if [ -d "../build" ]; then
	rm -r ../build
fi
mv build ../build
