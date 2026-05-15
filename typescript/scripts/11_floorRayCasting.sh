#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/floor_gloss.mgf -raytracing-method RayCasting \
  -obf output/11_floorRayCasting.bin \
  -nqcdivs 16 \
  -iterations 9 -radiance-method Galerkin \
  -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
  -raycast -raytracing-image-savefile ./output/11_floorRayCasting.ppm \
  -gr-min-elem-area 1e-8 -gr-link-error-threshold 1e-7 \
  "$@"
