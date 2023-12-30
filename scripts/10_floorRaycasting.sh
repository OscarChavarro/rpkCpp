#!/bin/bash

mkdir -p output

./build/rpk etc/floor_gloss.mgf -raytracing-method RayMatting \
    -nqcdivs 16 -iterations 9 -radiance-method RandomWalk \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -raytracing-image-savefile ./output/floorGlossRayMatting.ppm

./build/rpk etc/floor_gloss.mgf -raytracing-method RayCasting \
    -nqcdivs 16 \
    -iterations 9 -radiance-method RandomWalk \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -raytracing-image-savefile ./output/floorGlossRayCasting.ppm

./build/rpk etc/floor_gloss.mgf -raytracing-method BidirectionalPathTracing \
    -nqcdivs 16 -iterations 11 -radiance-method RandomWalk \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -raytracing-image-savefile ./output/floorGlossBidirectionalPathTracing.ppm \
    -rts-samples-per-pixel 100 # quite noise free at 3000 iterations

./build/rpk etc/floor_gloss.mgf -raytracing-method StochasticRaytracing \
    -nqcdivs 16 \
    -iterations 9 -radiance-method RandomWalk \
    -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
    -batch -batch-quit-at-end -offscreen \
    -raycast -raytracing-image-savefile ./output/floorGlossStochasticRaytracing.ppm \
    -rts-samples-per-pixel 100 # quite noise free at 3000 iterations
