#!/bin/bash

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
RPK_ROOT_DIR=$(CDPATH= cd -- "${PROJECT_DIR}/.." && pwd)

export RPK_GRADLE="gradle"
export RPK_GRADLE_QUIET="-q"
export RPK_APP_TASK=":testsuite:ApplicationCases:RenderparkApplication:run"
export RPK_ETC_DIR="${RPK_ROOT_DIR}/etc"
