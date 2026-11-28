#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/hospital/hosp.mgf \
  -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
  -eyepoint 1.1769 -0.045 1.5556 -center 5.2872 9.0366 0.9494 \
  -raycast -save-last-only -radiance-image-savefile ./output/03_hospital.png \
  "$@"
