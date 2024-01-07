#!/bin/bash

mkdir -p output
./build/rpk etc/corridor.mgf \
    -raytracing-method none \
    -nqcdivs 18 -iterations 100 -radiance-method Galerkin \
    -radiance-model-savefile output/02_corridor.wrl \
    -eyepoint -3.66 -5.52 7.2 -center 0.2 3.47 5.11 \
    -dont-force-onesided \
    -raycast -radiance-image-savefile ./output/02_corridor.ppm
