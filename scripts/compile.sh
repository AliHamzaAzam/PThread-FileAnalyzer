#!/bin/bash
mkdir -p ../bin

for cpp_file in ../src/*.cpp; do
  exe_name=$(basename "$cpp_file" .cpp)
  echo "Compiling $cpp_file to bin/$exe_name"
  g++-14 -std=c++17 -pthread -O3 -march=native  "$cpp_file" -o "../bin/$exe_name"
done