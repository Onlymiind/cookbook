#!/usr/bin/bash

cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .
cd ..