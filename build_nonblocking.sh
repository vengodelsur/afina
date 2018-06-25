#!/bin/bash
rm -r -f "build"
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
../itest/network_test.pl
./src/afina --network nonblocking
