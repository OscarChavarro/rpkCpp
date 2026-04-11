#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/hospital/hosp.mgf \
  -obf output/03_hospital.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/03_hospital.wrl \
  -eyepoint 1.1769 -0.045 1.5556 -center 5.2872 9.0366 0.9494 \
  -raycast -radiance-image-savefile ./output/03_hospital.ppm \
  "$@"
