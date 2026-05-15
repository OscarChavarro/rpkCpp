#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/floor_gloss.mgf -raytracing-method StochasticRaytracing \
    -obf output/14_floorStochasticRaytracingJacobi.bin \
    -nqcdivs 16 \
    -iterations 9 -radiance-method StochJacobi \
    -eyepoint 9.16 3.0 0.81 -center -2.72 1.63 -0.44 -updir 0 0 1 \
    -raycast -raytracing-image-savefile \
    ./output/14_floorStochasticRaytracingJacobi.ppm \
    -rts-samples-per-pixel 10 # quite noise free at 3000 iterations"
