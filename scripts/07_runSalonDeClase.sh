#!/bin/bash

mkdir -p output

./build/rpk ./etc/salon/classroom.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/classroom.wrl \
    -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/07_classroom.ppm
