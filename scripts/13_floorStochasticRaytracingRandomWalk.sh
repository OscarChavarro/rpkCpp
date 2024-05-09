#!/bin/bash

mkdir -p output

./build/rpk etc/floor_gloss.mgf -raytracing-method StochasticRaytracing \
    -nqcdivs 16 \
    -iterations 9 -radiance-method RandomWalk \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile \
    ./output/13_floorStochasticRaytracingRandomWalk.ppm \
    -rts-samples-per-pixel 100 # quite noise free at 3000 iterations
