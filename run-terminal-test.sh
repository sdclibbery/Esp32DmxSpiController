#!/bin/bash
# Build as a local executable to allow testing the effects

g++ -std=c++11 terminal-test.cpp sketch/modes.cpp sketch/palettes.cpp sketch/perlin.cpp sketch/hsv.cpp -lm -o terminal-test.exe
./terminal-test.exe
