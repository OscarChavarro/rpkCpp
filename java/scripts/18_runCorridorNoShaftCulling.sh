#!/bin/bash
# The corridor of 02_runCorridor.sh with shaft culling off: every Galerkin
# shadow ray that misses the shadow cache goes to the full-scene index
# (doc/vitralAlignmentPlan.md, step 4).
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output
"${RPK_GRADLE}" "${RPK_GRADLE_QUIET}" "${RPK_APP_TASK}" --args "${RPK_ETC_DIR}/corridor.mgf \
    -raytracing-method none \
    -nqcdivs 18 -iterations 21 -radiance-method Galerkin \
    -gr-shaft-culling never \
    -eyepoint -3.66 -5.52 7.2 -center 0.2 3.47 5.11 \
    -dont-force-onesided \
    -raycast -radiance-image-savefile ./output/18_corridorNoShaftCulling.ppm"
