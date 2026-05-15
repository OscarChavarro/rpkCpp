#!/bin/bash
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/office2/office2.mgf \
    -obf output/05_office2.bin \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -radiance-model-savefile output/05_office2.wrl \
    -eyepoint 1.43 5.89 2 -center 4.11 -3.7 0.7 \
    -raycast -radiance-image-savefile ./output/05_office2.ppm"
