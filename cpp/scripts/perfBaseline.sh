#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
PERF_DIR="${PROJECT_DIR}/output/perf"
STAMP=$(date +"%Y%m%d_%H%M%S")
REPORT="${PERF_DIR}/corridor_${STAMP}.txt"
RUN_LOG="${PERF_DIR}/corridor_${STAMP}.run.log"
TIME_LOG="${PERF_DIR}/corridor_${STAMP}.time.log"

mkdir -p "${PERF_DIR}"

(
    cd "${PROJECT_DIR}"
    /usr/bin/time -v "${SCRIPT_DIR}/02_runCorridor.sh" >"${RUN_LOG}" 2>"${TIME_LOG}"
)

{
    echo "Renderpark corridor perf baseline"
    echo "timestamp=${STAMP}"
    echo "script=cpp/scripts/02_runCorridor.sh"
    echo
    echo "[time]"
    awk '
        /Elapsed \(wall clock\) time/ { print "wall=" $0 }
        /User time \(seconds\)/ { print "user_seconds=" $NF }
        /System time \(seconds\)/ { print "system_seconds=" $NF }
        /Percent of CPU this job got/ { print "cpu_percent=" $NF }
        /Maximum resident set size/ { print "peak_rss_kb=" $NF }
    ' "${TIME_LOG}"
    echo
    echo "[app counters]"
    grep -Ei \
        'cpu|time|iteration|element|cluster|interaction|shadow|cache|ray' \
        "${RUN_LOG}" || true
    echo
    echo "run_log=${RUN_LOG}"
    echo "time_log=${TIME_LOG}"
} >"${REPORT}"

echo "${REPORT}"
