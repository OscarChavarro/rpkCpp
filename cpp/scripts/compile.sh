#!/bin/bash

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

cmake -S "${PROJECT_DIR}" -B "${PROJECT_DIR}/build"
cmake --build "${PROJECT_DIR}/build" -j 72
