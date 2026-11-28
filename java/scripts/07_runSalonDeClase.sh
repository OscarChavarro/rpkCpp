#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/salon/classroom.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
    -raycast -save-last-only -radiance-image-savefile ./output/07_classroom.png"
