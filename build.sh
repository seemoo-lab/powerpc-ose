#!/bin/bash
mkdir -p build
cd build
cmake ..
make
cd ..
rm -f elfpatch
ln -s build/elfpatch

