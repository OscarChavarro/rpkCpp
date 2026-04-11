#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry dist/vsdk/toolkit/app/Main.js -- \
  ../etc/office1/graz.mgf \
  -obf output/04_office1.bin \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -radiance-model-savefile output/04_office1.wrl \
  -eyepoint 3.7311 -0.011 2.3034 -center 1.0023 8.9229 -1.113 \
  -dont-force-onesided \
  -raycast -radiance-image-savefile ./output/04_office1.ppm \
  "$@"
