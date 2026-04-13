#!/bin/bash

mkdir -p output

./build/rpk ../etc/cube.mgf \
    -obf output/01_cubeGaussSiedel.bin \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -gr-iteration-method gaussseidel \
    -radiance-model-savefile output/01_cubeGaussSiedel.wrl \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -radiance-image-savefile ./output/01_cubeGaussSiedel.ppm
