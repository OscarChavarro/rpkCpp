#!/bin/sh
# Step 4 replay, Java port (doc/vitralAlignmentPlan.md, step 4 item 4):
# replays the step 1 ray corpus of each image through the legacy voxel grid,
# VITRAL's linear reference, binned SAH index and ported voxel grid, over the
# scene its own corpus.sh script builds.
#
#   replayRays.sh <corpus directory> image ...
#
# Runs each image's script with the RayCorpusReplay application
# (testsuite/ApplicationCases/RayCorpusReplay) as RPK_APP_TASK, or the task
# named by RPK_REPLAY_TASK. Reads <corpus directory>/<image>.vqc, written by
# this port's captureRays.sh or by the C++ one (same little-endian format;
# for instance ../cpp/rayCorpus, which is only read), and writes
# output/<image>.replay.txt (the summary) and output/<image>.replayReport.txt
# (every disagreement with its ray). RPK_RAY_REPLAY_THREADS,
# RPK_RAY_REPLAY_LINEAR_STRIDE and RPK_RAY_REPLAY_LIMIT pass through (see the
# application's Main.java); the threaded pass is off unless
# RPK_RAY_REPLAY_THREADS is above 1. Only the corpora with full-scene queries
# have something to replay: 12 and 14 (raytracers) and 16 and 18 (Galerkin
# with shaft culling off). A C++ corpus replays without capture mismatches
# only where the Java scene and arithmetic match the C++ ones bit for bit.
# Run from java/.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
. "$SCRIPT_DIR/corpus.sh"

CORPUS_DIR=$(CDPATH= cd -- "${1:?usage: replayRays.sh <corpus directory> image ...}" && pwd)
shift
cd "$PROJECT_DIR" || exit 2
REPLAY_TASK="${RPK_REPLAY_TASK:-:testsuite:ApplicationCases:RayCorpusReplay:run}"
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
    RPK_APP_TASK="$REPLAY_TASK" RPK_RAY_REPLAY_FILE="$CORPUS_DIR/$image.vqc" \
        RPK_RAY_REPLAY_REPORT="$PROJECT_DIR/output/$image.replayReport.txt" \
        bash "$SCRIPT_DIR/$script.sh" >"output/$image.replay.txt" 2>"output/$image.replay.err" || status=1
    sed -n '/^Ray corpus replay:/,/PASSED\|FAILED/p' "output/$image.replay.txt"
done
exit $status
