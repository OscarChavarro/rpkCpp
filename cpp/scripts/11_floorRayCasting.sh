#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/floor_gloss.mgf -raytracing-method RayCasting \
    -nqcdivs 16 \
    -iterations 9 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile ./output/11_floorRayCasting.png \
    -gr-min-elem-area 1e-8 -gr-link-error-threshold 1e-7
