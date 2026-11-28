#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/hospital/hosp.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
    -eyepoint 1.1769 -0.045 1.5556 -center 5.2872 9.0366 0.9494 \
    -raycast -save-last-only -radiance-image-savefile ./output/03_hospital.png"
