#!/bin/bash

./build/rpk ./etc/salon/classroom.mgf \
    -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
    -display-lists -iterations 9 \
    -radiance-model-savefile output/classroom.wrl -radiance-method Galerkin -raytracing-method none \
    -batch -batch-quit-at-end -offscreen \
    -raycast -radiance-image-savefile ./output/classroom.ppm
