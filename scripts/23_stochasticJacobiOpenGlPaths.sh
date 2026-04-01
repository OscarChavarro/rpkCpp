#!/bin/bash

# Runs Stochastic Jacobi with GLUT debug view enabled so we can inspect
# the final precomputed result (no incremental OpenGL drawing during doStep).

mkdir -p output

./build/rpk etc/cube.mgf \
    -obf output/23_stochasticJacobiOpenGlPaths.bin \
    -raytracing-method none \
    -iterations 2 \
    -radiance-method StochJacobi \
    -srr-display importance \
    -srr-hierarchical yes \
    -srr-clustering oriented \
    -eyepoint 4.78 -10.7 8 \
    -center 4.8 -1 5.62 \
    -glutDebug
