#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/office1/graz.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 3.7311 -0.011 2.3034 -center 1.0023 8.9229 -1.113 \
    -dont-force-onesided \
    -raycast -save-last-only -radiance-image-savefile ./output/04_office1.png"
