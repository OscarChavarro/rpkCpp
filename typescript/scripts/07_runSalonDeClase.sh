#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/salon/classroom.mgf \
  -obf output/07_classroom.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/07_classroom.wrl \
  -eyepoint 3.2 12.5 2.3 -center 4.5 2.9 0.15 \
  -raycast -radiance-image-savefile ./output/07_classroom.ppm \
  "$@"
