#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/soda.mgf \
  -raytracing-method none -iterations 11 -radiance-method Galerkin -gr-iteration-method parallel-jacobi \
  -eyepoint 0 1 -3 -center 0 0.5 0 -updir 0 1 0 \
  -raycast -save-last-only -radiance-image-savefile ./output/08_soda.png \
  "$@"
