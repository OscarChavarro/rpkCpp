#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/cube.mgf \
  -raytracing-method none -iterations 11 -radiance-method Galerkin \
  -gr-iteration-method southwell \
  -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
  -raycast -save-last-only -radiance-image-savefile output/01_cubeSouthwell.ppm \
  "$@"
