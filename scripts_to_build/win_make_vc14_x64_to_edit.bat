mkdir build
cd build
mkdir x64
cd x64

rem "Before running this script please make sure the appropriate paths are set in CMakeLists.txt"

cmake -G "Visual Studio 14 Win64" ../../..

cmake --build . --config Release

PAUSE