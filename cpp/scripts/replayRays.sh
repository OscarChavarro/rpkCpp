#!/bin/sh
# Step 4 replay (doc/vitralAlignmentPlan.md, step 4 item 4): replays the step 1
# ray corpus of each image through VITRAL's linear reference, binned SAH index
# and ported voxel grid, compared with the captured answers, sequentially and
# from several threads, over the scene its own corpus.sh script builds.
#
#   replayRays.sh <corpus directory> image ...
#
# Uses the rayCorpusReplay program of the default build
# (testsuite/ApplicationCases/RayCorpusReplay/build), or RPK_REPLAY_BIN and
# RPK_LIB_DIR when set (for instance a thread sanitizer build). Reads
# <corpus directory>/<image>.vqc and writes output/<image>.replay.txt (the
# summary) and output/<image>.replayReport.txt (every disagreement with its
# ray). RPK_RAY_REPLAY_THREADS, RPK_RAY_REPLAY_LINEAR_STRIDE and
# RPK_RAY_REPLAY_LIMIT pass through (see the program's Main.cpp). Only the
# corpora with full-scene queries have something to replay: 12 and 14
# (raytracers) and 01 and 02 (Galerkin, captured with shaft culling off, the
# default since 2026-11-21); with shaft culling the Galerkin scenes answer
# every shadow ray from the cache or a restricted list. Run from a directory
# whose parent holds etc/, like the other scripts.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
. "$SCRIPT_DIR/corpus.sh"

CORPUS_DIR=$(CDPATH= cd -- "${1:?usage: replayRays.sh <corpus directory> image ...}" && pwd)
shift
REPLAY_BIN="${RPK_REPLAY_BIN:-$PROJECT_DIR/testsuite/ApplicationCases/RayCorpusReplay/build/rayCorpusReplay}"
if [ ! -x "$REPLAY_BIN" ]; then
    echo "No replay program at $REPLAY_BIN"
    exit 2
fi
mkdir -p output

selection=$(corpusSelect "$@")
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

status=0
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    script=$(echo "$line" | cut -d'|' -f2)
    if [ ! -f "$CORPUS_DIR/$image.vqc" ]; then
        echo "$image: no corpus file $CORPUS_DIR/$image.vqc (regenerate it with captureRays.sh)"
        status=1
        continue
    fi
    RPK_BIN="$REPLAY_BIN" RPK_RAY_REPLAY_FILE="$CORPUS_DIR/$image.vqc" \
        RPK_RAY_REPLAY_REPORT="output/$image.replayReport.txt" \
        bash "$SCRIPT_DIR/$script.sh" >"output/$image.replay.txt" 2>"output/$image.replay.err" || status=1
    sed -n '/^Ray corpus replay:/,/PASSED\|FAILED/p' "output/$image.replay.txt"
done
exit $status
