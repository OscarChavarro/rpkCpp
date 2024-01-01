#!/bin/bash

mkdir -p output

./build/rpk etc/cube.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/cube.wrl \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/01_cube.ppm
