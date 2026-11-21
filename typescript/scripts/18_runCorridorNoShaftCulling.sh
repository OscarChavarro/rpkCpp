#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output

node scripts/runNodeProgram.mjs --build --entry testsuite/ApplicationCases/RenderparkApplication/dist/vsdk/toolkit/app/Main.js -- \
  ../etc/corridor.mgf \
  -raytracing-method none \
  -nqcdivs 18 -iterations 21 -radiance-method Galerkin \
  -gr-shaft-culling never \
  -eyepoint -3.66 -5.52 7.2 -center 0.2 3.47 5.11 \
  -dont-force-onesided \
  -raycast -radiance-image-savefile ./output/18_corridorNoShaftCulling.ppm \
  "$@"
