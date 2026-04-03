#!/bin/bash

g++ -o rpkProfile -pg -Wunused-variable -Wunused-but-set-variable -Wunused-function -pedantic -std=c++11 -O3 -Isrc `find ./src -name "*.cpp"` -lglut -lGLU -lGL -lOSMesa -lX11 -lm

#g++ -o rpkDebug -g -Wunused-variable -Wunused-but-set-variable -Wunused-function -pedantic -std=c++11 -O3 -Isrc `find ./src -name "*.cpp"` -lGLU -lGL -lOSMesa -lX11 -lm
