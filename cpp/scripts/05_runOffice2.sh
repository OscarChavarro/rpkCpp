#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/office2/office2.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 1.43 5.89 2 -center 4.11 -3.7 0.7 \
    -raycast -save-last-only -radiance-image-savefile ./output/05_office2.ppm
