#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/cube.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -gr-iteration-method southwell \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -save-last-only -radiance-image-savefile ./output/01_cubeSouthwell.ppm
