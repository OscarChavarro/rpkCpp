#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/cube.mgf \
  -obf output/01_cubeGaussSiedel.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -gr-iteration-method gaussseidel \
  -radiance-model-savefile output/01_cubeGaussSiedel.wrl \
  -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
  -raycast -radiance-image-savefile output/01_cubeGaussSiedel.ppm \
  "$@"
