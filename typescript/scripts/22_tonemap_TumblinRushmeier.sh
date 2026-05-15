#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/cube.mgf \
  -obf output/22_tonemapTumblinRushmeier.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/01_cube.wrl \
  -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
  -raycast -radiance-image-savefile ./output/22_tonemapTumblinRushmeier.ppm \
  -gr-min-elem-area 1e-9 -gr-link-error-threshold 1e-8 -tonemapping TumblinRushmeier \
  "$@"
