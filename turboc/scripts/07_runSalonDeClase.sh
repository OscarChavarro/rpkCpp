#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/salon/classroom.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
    -raycast -save-last-only -radiance-image-savefile ./output/07_classroom.png
