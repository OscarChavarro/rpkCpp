#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output
"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/corridor.mgf \
    -raytracing-method none \
    -nqcdivs 18 -iterations 21 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint -3.66 -5.52 7.2 -center 0.2 3.47 5.11 \
    -dont-force-onesided \
    -raycast -save-last-only -radiance-image-savefile ./output/02_corridor.ppm"
