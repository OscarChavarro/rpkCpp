#!/bin/sh
# Image gate of the test corpus. Run from turboc/ after scripts/runCorpus.sh.
#
#   testReviewResults.sh [--capture-references] [image ...]
#
# With no option every output must equal its reference image
# etc/goldenImagesForTests/<image>.png (compare -metric AE = 0).
# --capture-references stores the current outputs as the reference images.
# The golden images of the project are the cpp port's
# (../cpp/etc/goldenImagesForTests): compare the references with them
# (compare -metric AE and RMSE) before capturing.
#
# Images may be selected by image or script name; the default is the whole
# corpus plus the scenes outside it (corpus.sh OPTIONAL_TABLE) whose run has
# finished, as output/<log>.status shows; an unfinished one is reported as
# NOT COMPARED, a failed run as a failure. Difference images of failed
# comparisons go to output/diff/.
# Turbo C++ port note: there is no parallel Random Walk (the cpp port's
# 50% size RMSE bound of case 13 does not apply), so every image is compared
# with AE = 0.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

golden_dir="etc/goldenImagesForTests"
diff_dir="output/diff"

mode="check"
case "${1:-}" in
    --capture-references) mode="capture"; shift ;;
    --*) echo "Unknown option $1"; exit 2 ;;
esac

if ! command -v compare >/dev/null 2>&1; then
    echo "*** TEST ERROR! *** ImageMagick 'compare' is not installed"
    exit 2
fi

selection=$(corpusSelect "$@")
# With no image named, the finished scenes outside the corpus are checked too
if [ $# -eq 0 ]; then
    selection=$(printf '%s\n%s\n' "$selection" "$OPTIONAL_TABLE")
fi
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

failures=0
fail() {
    echo "FAIL: $*"
    failures=$((failures + 1))
}

# Prints the metric value compare reports; for RMSE the normalized value
# in parentheses. Returns compare's exit status (2 means error).
runCompare() {
    metric="$1"
    first="$2"
    second="$3"
    target="$4"
    result=$(compare -metric "$metric" "$first" "$second" "$target" 2>&1)
    status=$?
    if [ "$metric" = "RMSE" ]; then
        result=$(echo "$result" | sed -n 's/.*(\(.*\)).*/\1/p')
    fi
    echo "$result"
    return $status
}

checkImage() {
    image="$1"
    output="output/$image.png"
    golden="$golden_dir/$image.png"

    if [ ! -f "$output" ]; then
        fail "$image: missing output $output"
        return
    fi
    if [ ! -f "$golden" ]; then
        fail "$image: missing golden image $golden"
        return
    fi

    ae=$(runCompare AE "$golden" "$output" null:)
    status=$?
    if [ $status -eq 2 ]; then
        fail "$image: compare error: $ae"
        return
    fi
    if [ $status -ne 0 ]; then
        mkdir -p "$diff_dir"
        compare "$golden" "$output" "$diff_dir/${image}_vs_golden.png" 2>/dev/null
        rmse=$(runCompare RMSE "$golden" "$output" null:)
        fail "$image: AE=$ae RMSE=$rmse against golden image (difference: $diff_dir/${image}_vs_golden.png)"
        return
    fi
    echo "MATCH: $image"
}

captureImage() {
    image="$1"
    output="output/$image.png"
    if [ ! -f "$output" ]; then
        fail "$image: missing output, not captured"
        return
    fi
    mkdir -p "$golden_dir"
    cp -p "$output" "$golden_dir/$image.png"
    echo "CAPTURED: $image"
}

# A scene outside the corpus is compared only once its run has finished
optionalReady() {
    image="$1"
    log="$2"
    if [ ! -f "output/$log.status" ]; then
        echo "NOT COMPARED: $image: its run has not finished (no output/$log.status); this scene was not compared"
        return 1
    fi
    if [ "$(cat "output/$log.status")" != "0" ]; then
        fail "$image: its run ended with exit status $(cat "output/$log.status") (see output/$log.err)"
        return 1
    fi
    return 0
}

for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    log=$(echo "$line" | cut -d'|' -f3)
    if isOptional "$line" && ! optionalReady "$image" "$log"; then
        continue
    fi
    case "$mode" in
        check) checkImage "$image" ;;
        capture) captureImage "$image" ;;
    esac
done

if [ "$failures" -eq 0 ]; then
    echo "TEST PASS"
    exit 0
fi

echo "*** TEST ERROR! *** $failures failure(s)"
exit 1
