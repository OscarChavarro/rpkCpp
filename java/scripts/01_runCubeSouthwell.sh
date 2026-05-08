#!/bin/bash

mkdir -p output

gradle run --args "../etc/cube.mgf \
    -obf output/01_cubeSouthwell.bin \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -gr-iteration-method southwell \
    -radiance-model-savefile output/01_cubeSouthwell.wrl \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -radiance-image-savefile output/01_cubeSouthwell.ppm"
