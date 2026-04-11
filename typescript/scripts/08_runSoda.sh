#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/soda.mgf \
  -obf output/08_soda.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/08_soda.wrl \
  -eyepoint 0 1 -3 -center 0 0.5 0 -updir 0 1 0 \
  -raycast -radiance-image-savefile ./output/08_soda.ppm \
  "$@"
