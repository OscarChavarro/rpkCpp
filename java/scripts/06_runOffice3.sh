#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/office3/office.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 2.52 3.59 -0.51 -center -2.64 1.94 1.63 -updir 0 1 0 \
    -raycast -save-last-only -radiance-image-savefile ./output/06_office3.png"
