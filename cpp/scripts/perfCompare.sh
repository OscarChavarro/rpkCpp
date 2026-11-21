#!/bin/sh
# T4 timing gate. Runs the corridor solver alternately from a reference tree
# and from this tree, so that machine load drifts affect both sides alike, and
# prints the median and the spread (minimum to maximum) of user CPU, wall
# clock and peak RSS for each side.
#
#   perfCompare.sh <reference cpp directory> [runs per side, default 5]
#
# The reference directory is the cpp/ of a built checkout of the reference
# commit (for instance a git worktree). Each side writes its images into its
# own output/ directory. Raw /usr/bin/time logs go to output/perf/.

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CURRENT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
REFERENCE_DIR=$(CDPATH= cd -- "${1:?usage: perfCompare.sh <reference cpp directory> [runs]}" && pwd)
RUNS="${2:-5}"
PERF_DIR="${CURRENT_DIR}/output/perf"
STAMP=$(date +"%Y%m%d_%H%M%S")
mkdir -p "$PERF_DIR"

runOnce() {
    side="$1"
    tree="$2"
    run="$3"
    log="$PERF_DIR/compare_${STAMP}_${side}_${run}.time.log"
    (cd "$tree" && /usr/bin/time -v bash scripts/02_runCorridor.sh >/dev/null 2>"$log")
    user=$(awk '/User time \(seconds\)/ { print $NF }' "$log")
    wall=$(awk '/Elapsed \(wall clock\) time/ { n = split($NF, t, ":"); s = 0; for (i = 1; i <= n; i++) s = s * 60 + t[i]; print s }' "$log")
    rss=$(awk '/Maximum resident set size/ { print $NF }' "$log")
    echo "$side $user $wall $rss" >>"$PERF_DIR/compare_${STAMP}.txt"
    echo "$side run $run: user ${user}s wall ${wall}s peak RSS ${rss} kB"
}

i=1
while [ "$i" -le "$RUNS" ]; do
    runOnce reference "$REFERENCE_DIR" "$i"
    runOnce current "$CURRENT_DIR" "$i"
    i=$((i + 1))
done

summary() {
    side="$1"
    column="$2"
    name="$3"
    awk -v s="$side" -v c="$column" '$1 == s { print $c }' "$PERF_DIR/compare_${STAMP}.txt" | sort -g |
        awk -v s="$side" -v n="$name" '{ v[NR] = $1 } END {
            m = (NR % 2) ? v[(NR + 1) / 2] : (v[NR / 2] + v[NR / 2 + 1]) / 2
            printf "%-9s %-10s median %12.2f  min %12.2f  max %12.2f  spread %6.2f%%\n", s, n, m, v[1], v[NR], 100 * (v[NR] - v[1]) / m
        }'
}

echo
for side in reference current; do
    summary "$side" 2 user
    summary "$side" 3 wall
    summary "$side" 4 peakRssKb
done
echo "raw results: $PERF_DIR/compare_${STAMP}.txt"
