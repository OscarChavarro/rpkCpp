#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/office3/office.mgf \
  -obf output/06_office3.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/06_office3.wrl \
  -eyepoint 2.52 3.59 -0.51 -center -2.64 1.94 1.63 -updir 0 1 0 \
  -raycast -radiance-image-savefile ./output/06_office3.ppm \
  "$@"
