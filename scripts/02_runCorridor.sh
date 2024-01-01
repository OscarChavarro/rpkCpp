#!/bin/bash

mkdir -p output
./build/rpk etc/corridor.mgf \
    -raytracing-method none \
    -nqcdivs 16 -iterations 100 -radiance-method Galerkin \
    -radiance-model-savefile output/corridor.wrl \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/02_corridor.ppm
