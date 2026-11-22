#!/bin/bash

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

# RPK_LIB_DIR and RPK_BIN may be set beforehand to run another build of the
# program (for instance the ray capture build used by scripts/captureRays.sh)
RPK_LIB_DIR="${RPK_LIB_DIR:-${PROJECT_DIR}/base/build:${PROJECT_DIR}/rpkBase/build}"
export LD_LIBRARY_PATH="${RPK_LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export RPK_BIN="${RPK_BIN:-${PROJECT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build/rpk}"
