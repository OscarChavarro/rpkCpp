#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/cube.mgf \
    -obf output/21_tonemapWard.bin \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/01_cube.wrl \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -radiance-image-savefile ./output/21_tonemapWard.ppm \
    -gr-min-elem-area 1e-9 -gr-link-error-threshold 1e-8 -tonemapping Ward"
