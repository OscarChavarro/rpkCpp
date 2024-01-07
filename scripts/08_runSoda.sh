#!/bin/bash

mkdir -p output

./build/rpk ./etc/soda.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/08_soda.wrl \
    -eyepoint 0 1 -3 -center 0 0.5 0 -updir 0 1 0 \
    -raycast -radiance-image-savefile ./output/08_soda.ppm
