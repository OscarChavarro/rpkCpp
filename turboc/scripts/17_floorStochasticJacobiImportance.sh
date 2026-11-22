#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/floor_gloss.mgf -raytracing-method StochasticRaytracing \
    -nqcdivs 16 \
    -iterations 9 -radiance-method StochJacobi -srr-importance-driven yes \
    -eyepoint 9.16 3.0 0.81 -center -2.72 1.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile \
    ./output/17_floorStochasticJacobiImportance.ppm \
    -rts-samples-per-pixel 10
