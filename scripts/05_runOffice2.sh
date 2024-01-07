#!/bin/bash

mkdir -p output

./build/rpk etc/office2/office2.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/05_office2.wrl \
    -eyepoint 1.43 5.89 2 -center 4.11 -3.7 0.7 \
    -raycast -radiance-image-savefile ./output/05_office2.ppm
