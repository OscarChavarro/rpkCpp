#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/cube.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -save-last-only -radiance-image-savefile ./output/21_tonemapWard.png \
    -gr-min-elem-area 1e-9 -gr-link-error-threshold 1e-8 -tonemapping Ward"
