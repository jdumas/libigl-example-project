#! /bin/bash
# export CC=/usr/lib/emscripten/emcc
# export CXX=/usr/lib/emscripten/em++

rm -rf build
mkdir build
pushd build
#cmake -DCMAKE_TOOLCHAIN_PATH=/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake ..
cmake ..
#make VERBOSE=1
make
popd
