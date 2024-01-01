#!/bin/bash

mkdir -p output

./build/rpk etc/hospital/hosp.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/hospital.wrl \
    -eyepoint 1.1769 -0.045 1.5556 -center 5.2872 9.0366 0.9494 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/03_hospital.ppm
