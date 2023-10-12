#!/usr/bin/env bash

# Show commands
set -x
# Stop script if an error occurs
set -e

# Build the executable with CMake
#-DCHECK_CODE=ON -DFORMAT_CODE=ON
cmake -S . -B build_cmake "$@"
cmake --build build_cmake

./build_cmake/aniFileExtractor test/test.ani test/out_test_images
./build_cmake/aniFileExtractor ani test/test.ani
./build_cmake/aniFileExtractor png test/test.png
./build_cmake/aniFileExtractor ico test/test.ico

# Build the executable with gcc
mkdir -p build_gcc
g++ aniFileExtractor.cpp -I ./ -std=c++23 -o build_gcc/aniFileExtractor

./build_gcc/aniFileExtractor test/test.ani test/out_test_images
./build_gcc/aniFileExtractor ani test/test.ani
./build_gcc/aniFileExtractor png test/test.png
./build_gcc/aniFileExtractor ico test/test.ico

# Build the executable with clang
mkdir -p build_clang
clang++ aniFileExtractor.cpp -I ./ -std=c++20 -o build_clang/aniFileExtractor

./build_clang/aniFileExtractor test/test.ani test/out_test_images
./build_clang/aniFileExtractor ani test/test.ani
./build_clang/aniFileExtractor png test/test.png
./build_clang/aniFileExtractor ico test/test.ico
