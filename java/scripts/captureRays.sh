#!/bin/sh
# Step 1 capture, Java port: runs corpus scenes with ray capture enabled and
# stores, per scene, the ray accounting (where each shadow ray is answered)
# and the ray corpus of every list and index visibility query.
#
#   captureRays.sh <corpus directory> image ...
#
# The C++ port needs a build configured WITH_RAY_CAPTURE; the Java port turns
# the same code paths on at run time (RPK_RAY_CAPTURE=ON, see
# VisibilityQueryRecorder.captureEnabled), so it uses the normal build. Run
# from java/; images go to its output/. For each image the corpus directory
# receives <image>.vqc (format in VisibilityQueryRecorder, little-endian,
# identical to the C++ one) and output/rayCorpus receives
# <image>.accounting.txt. Do not point the corpus directory at cpp/rayCorpus:
# it would overwrite the C++ corpora.
# The runs use -scene-acceleration legacy (through a wrapper of gradle in
# output/ that appends it to the application arguments), so that a corpus
# holds the answers of RenderPark's original voxel grid.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
. "$SCRIPT_DIR/corpus.sh"

CORPUS_DIR="${1:?usage: captureRays.sh <corpus directory> image ...}"
shift
mkdir -p "$CORPUS_DIR"
CORPUS_DIR=$(CDPATH= cd -- "$CORPUS_DIR" && pwd)
cd "$PROJECT_DIR" || exit 2
ACCOUNTING_DIR="$PROJECT_DIR/output/rayCorpus"
mkdir -p "$ACCOUNTING_DIR" output

GRADLE_BIN="${RPK_GRADLE:-gradle}"
wrapper="$PROJECT_DIR/output/captureGradle.sh"
cat >"$wrapper" <<WRAPPER
#!/bin/bash
args=()
appendNext=0
for argument in "\$@"; do
    if [ \$appendNext -eq 1 ]; then
        argument="\$argument -scene-acceleration legacy"
        appendNext=0
    elif [ "\$argument" = "--args" ]; then
        appendNext=1
    fi
    args+=("\$argument")
done
exec "$GRADLE_BIN" "\${args[@]}"
WRAPPER
chmod +x "$wrapper"

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
    RPK_GRADLE="$wrapper" RPK_RAY_CAPTURE=ON \
        RPK_RAY_CAPTURE_FILE="$CORPUS_DIR/$image.vqc" \
        bash "$SCRIPT_DIR/$script.sh" >"output/$log.log" 2>"output/$log.err" || status=1
    grep '^Ray capture: ' "output/$log.err" | grep -v 'corpus written to' | sed 's/^Ray capture: //' >"$ACCOUNTING_DIR/$image.accounting.txt"
    echo "$image:"
    sed 's/^/  /' "$ACCOUNTING_DIR/$image.accounting.txt"
done
exit $status
