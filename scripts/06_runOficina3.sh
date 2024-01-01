#!/bin/bash

mkdir -p output

./build/rpk etc/office3/office.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/office3.wrl \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/06_office3.ppm
