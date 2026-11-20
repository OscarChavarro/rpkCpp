#!/bin/sh
# Step 1 capture: runs corpus scenes with a build configured WITH_RAY_CAPTURE
# and stores, per scene, the ray accounting (where each shadow ray is
# answered) and the ray corpus of every list and index visibility query.
#
#   captureRays.sh <capture build directory> <corpus directory> image ...
#
# The capture build directory is the -B directory of
#   cmake -S cpp -B <dir>/top -DWITH_RAY_CAPTURE=ON -DRPK_BASE_BINARY_DIR=<dir>/base \
#     -DRPK_OPENGL_BINARY_DIR=<dir>/opengl -DRPK_RENDERPARK_BINARY_DIR=<dir>/rpk
# without the /top suffix. Run from a directory whose parent holds etc/, like
# the other scripts; images go to its output/. For each image the corpus
# directory receives <image>.vqc (format in VisibilityQueryRecorder.h) and
# <image>.accounting.txt.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

BUILD_DIR=$(CDPATH= cd -- "${1:?usage: captureRays.sh <capture build directory> <corpus directory> image ...}" && pwd)
CORPUS_DIR="${2:?usage: captureRays.sh <capture build directory> <corpus directory> image ...}"
shift 2
mkdir -p "$CORPUS_DIR" output
CORPUS_DIR=$(CDPATH= cd -- "$CORPUS_DIR" && pwd)

if [ ! -x "$BUILD_DIR/rpk/rpk" ]; then
    echo "No capture build at $BUILD_DIR/rpk/rpk"
    exit 2
fi

selection=$(corpusSelect "$@")
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

status=0
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    script=$(echo "$line" | cut -d'|' -f2)
    log=$(echo "$line" | cut -d'|' -f3)
    RPK_BIN="$BUILD_DIR/rpk/rpk" RPK_LIB_DIR="$BUILD_DIR/base:$BUILD_DIR/opengl" \
        RPK_RAY_CAPTURE_FILE="$CORPUS_DIR/$image.vqc" \
        bash "$SCRIPT_DIR/$script.sh" >"output/$log.log" 2>"output/$log.err" || status=1
    grep '^Ray capture: ' "output/$log.err" | grep -v 'corpus written to' | sed 's/^Ray capture: //' >"$CORPUS_DIR/$image.accounting.txt"
    echo "$image:"
    sed 's/^/  /' "$CORPUS_DIR/$image.accounting.txt"
done
exit $status
