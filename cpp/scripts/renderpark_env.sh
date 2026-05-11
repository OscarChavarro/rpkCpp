#!/bin/bash

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

export LD_LIBRARY_PATH="${PROJECT_DIR}/base/build:${PROJECT_DIR}/opengl/build${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export RPK_BIN="${PROJECT_DIR}/testsuite/ApplicationCases/RenderparkApplication/build/rpk"
