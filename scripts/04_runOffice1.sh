#!/bin/bash

mkdir -p output

./build/rpk etc/office1/graz.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/04_office1.wrl \
    -eyepoint 3.7311 -0.011 2.3034 -center 1.0023 8.9229 -1.113 \
    -dont-force-onesided \
    -raycast -radiance-image-savefile ./output/04_office1.ppm
