#!/bin/bash
# The cube of 01_runCubeJacobi.sh with shaft culling off: every Galerkin
# shadow ray that misses the shadow cache goes to the full-scene index
# (doc/vitralAlignmentPlan.md, step 4).
source "$(dirname "$0")/renderpark_env.sh"

mkdir -p output

"${RPK_BIN}" ../etc/cube.mgf \
    -raytracing-method none -iterations 11 -radiance-method Galerkin \
    -gr-iteration-method jacobi -gr-shaft-culling never \
    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
    -raycast -radiance-image-savefile ./output/16_cubeJacobiNoShaftCulling.ppm
