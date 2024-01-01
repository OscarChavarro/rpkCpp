#!/bin/bash

mkdir -p output

./build/rpk etc/office2/office2.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/office2.wrl \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/05_office2.ppm
