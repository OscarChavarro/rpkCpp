#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/floor_gloss.mgf -raytracing-method RayMatting \
  -obf output/10_floorRayMatting.bin \
  -nqcdivs 16 -iterations 9 -radiance-method RandomWalk \
  -eyepoint 8.16 1.99 0.81 -center -1.72 2.63 -0.44 -updir 0 0 1 \
  -raycast -raytracing-image-savefile ./output/10_floorRayMatting.ppm \
  "$@"
