#!/usr/bin/env bash

wget https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
unzip glfw-3.4.zip
cd glfw-3.4
cmake -B build -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF
cmake --build build
sudo cmake --install build
cd ..

wget https://github.com/fmtlib/fmt/releases/download/12.0.0/fmt-12.0.0.zip
unzip fmt-12.0.0.zip
cd fmt-12.0.0
cmake -B build -DFMT_TEST=OFF
cmake --build build
sudo cmake --install build
cd ..
