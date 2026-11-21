#!/bin/bash

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

gradle -q :base:classes
gradle -q :testsuite:ApplicationCases:RenderparkApplication:classes
gradle -q :testsuite:ApplicationCases:RayCorpusReplay:classes
audit=$(python3 "${SCRIPT_DIR}/classAudit.py") || { echo "${audit}"; exit 1; }
