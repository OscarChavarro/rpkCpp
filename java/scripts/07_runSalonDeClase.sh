#!/bin/bash

mkdir -p output

gradle run --args "../etc/salon/classroom.mgf \
    -obf output/07_classroom.bin \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/07_classroom.wrl \
    -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
    -raycast -radiance-image-savefile ./output/07_classroom.ppm"
