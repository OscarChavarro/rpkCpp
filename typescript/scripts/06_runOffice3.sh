#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/office3/office.mgf \
  -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
  -eyepoint 2.52 3.59 -0.51 -center -2.64 1.94 1.63 -updir 0 1 0 \
  -raycast -save-last-only -radiance-image-savefile ./output/06_office3.png \
  "$@"
