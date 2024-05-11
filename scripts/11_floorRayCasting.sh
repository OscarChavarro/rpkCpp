#!/bin/bash

mkdir -p output

./build/rpk etc/floor_gloss.mgf -raytracing-method RayCasting \
    -nqcdivs 16 \
    -iterations 9 -radiance-method Galerkin \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile ./output/11_floorRayCasting.ppm \
    -gr-min-elem-area 1e-8 -gr-link-error-threshold 1e-7
