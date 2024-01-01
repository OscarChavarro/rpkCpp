#!/bin/bash

mkdir -p output

./build/rpk etc/office3/office.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/06_office3.wrl \
    -eyepoint 2.52 3.59 -0.51 -center -2.64 1.94 1.63 -updir 0 1 0 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/06_office3.ppm
