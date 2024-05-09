#!/bin/bash

mkdir -p output

./build/rpk etc/floor_gloss.mgf -raytracing-method StochasticRaytracing \
    -nqcdivs 16 \
    -iterations 9 -radiance-method StochJacobi \
    -eyepoint 9.16 3.0 0.81 -center -2.72 1.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile \
    ./output/14_floorStochasticRaytracingJacobi.ppm \
    -rts-samples-per-pixel 10 # quite noise free at 3000 iterations
