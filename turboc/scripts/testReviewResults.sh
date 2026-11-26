#!/bin/sh
# Image gate of the test corpus. Run from cpp/ after scripts/runCorpus.sh.
#
#   testReviewResults.sh [mode] [image ...]
#
# Modes:
#   (none)                both checks below
#   --exact               T1: every output must equal its port reference
#                         (compare -metric AE = 0) and its run log must give the
#                         reference statistics fingerprint
#   --golden              T3: compare -metric RMSE of every output against its
#                         golden image in ../cpp/etc/goldenImagesForTests must not exceed
#                         the bound recorded in etc/goldenImagesForTests/goldenBounds.txt
#   --capture-references  store the current outputs as port references, with
#                         checksums and fingerprints (unchanged tree only)
#   --record-bounds       record the current RMSE against each golden as its bound
#
# Images may be selected by image or script name; the default is the whole
# corpus plus the scenes outside it (corpus.sh OPTIONAL_TABLE) whose run has
# finished, as output/<log>.status shows; an unfinished one is reported as
# NOT COMPARED, a failed run as a failure. Difference images of failed comparisons go to output/diff/.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

golden_dir="../cpp/etc/goldenImagesForTests"
reference_dir="etc/goldenImagesForTests"
manifest="$reference_dir/manifest.txt"
bounds="$reference_dir/goldenBounds.txt"
diff_dir="output/diff"

mode="all"
case "${1:-}" in
    --exact) mode="exact"; shift ;;
    --golden) mode="golden"; shift ;;
    --capture-references) mode="capture"; shift ;;
    --record-bounds) mode="bounds"; shift ;;
    --refresh-baseline)
        echo "--refresh-baseline was removed: golden images change only by owner decision"
        exit 2 ;;
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

sha() {
    sha256sum "$1" | cut -d' ' -f1
}

checkExact() {
    image="$1"
    log="$2"
    output="output/$image.ppm"
    reference="$reference_dir/$image.ppm"

    if [ ! -f "$output" ]; then
        fail "$image: missing output $output"
        return
    fi

    if [ -f "$reference" ]; then
        ae=$(runCompare AE "$reference" "$output" null:)
        status=$?
        if [ $status -eq 2 ]; then
            fail "$image: compare error: $ae"
            return
        fi
        if [ $status -ne 0 ]; then
            mkdir -p "$diff_dir"
            compare "$reference" "$output" "$diff_dir/${image}_vs_reference.png" 2>/dev/null
            rmse=$(runCompare RMSE "$reference" "$output" null:)
            fail "$image: AE=$ae RMSE=$rmse against port reference (difference: $diff_dir/${image}_vs_reference.png)"
            return
        fi
    else
        expected=$(awk -v i="$image" '$1 == i { print $2 }' "$manifest" 2>/dev/null)
        if [ -z "$expected" ]; then
            fail "$image: no port reference"
            return
        fi
        if [ "$(sha "$output")" != "$expected" ]; then
            fail "$image: checksum differs from port reference (reference image not present locally)"
            return
        fi
    fi

    fingerprint="$reference_dir/fingerprints/$image.txt"
    if [ -f "$fingerprint" ]; then
        if [ ! -f "output/$log.log" ] || [ ! -f "output/$log.err" ]; then
            fail "$image: missing run logs output/$log.log and .err for the statistics fingerprint"
            return
        fi
        if ! statsFingerprint "output/$log" | diff -q "$fingerprint" - >/dev/null; then
            fail "$image: statistics fingerprint differs; first differences:"
            statsFingerprint "output/$log" | diff "$fingerprint" - | head -10
            return
        fi
    fi
    echo "EXACT: $image"
}

checkGolden() {
    image="$1"
    golden="$2"
    output="output/$image.ppm"

    if [ "$golden" = "-" ]; then
        echo "SKIP: $image has no golden image"
        return
    fi
    if [ ! -f "$output" ]; then
        fail "$image: missing output $output"
        return
    fi
    if [ ! -f "$golden_dir/$golden.ppm" ]; then
        fail "$image: missing golden image $golden_dir/$golden.ppm"
        return
    fi

    rmse=$(runCompare RMSE "$golden_dir/$golden.ppm" "$output" null:)
    if [ $? -eq 2 ] || [ -z "$rmse" ]; then
        fail "$image: compare error against golden $golden.ppm"
        return
    fi
    bound=$(awk -v i="$image" '$1 == i { print $3 }' "$bounds" 2>/dev/null)
    if [ -z "$bound" ]; then
        fail "$image: no recorded RMSE bound (measured $rmse)"
        return
    fi
    if awk -v r="$rmse" -v b="$bound" 'BEGIN { exit !(r + 0 > b + 0) }'; then
        mkdir -p "$diff_dir"
        compare "$golden_dir/$golden.ppm" "$output" "$diff_dir/${image}_vs_golden.png" 2>/dev/null
        fail "$image: RMSE $rmse above bound $bound against golden $golden.ppm (difference: $diff_dir/${image}_vs_golden.png)"
        return
    fi
    echo "GOLDEN: $image RMSE $rmse (bound $bound)"
}

captureReference() {
    image="$1"
    log="$2"
    output="output/$image.ppm"
    if [ ! -f "$output" ] || [ ! -f "output/$log.log" ] || [ ! -f "output/$log.err" ]; then
        fail "$image: missing output or log, not captured"
        return
    fi
    mkdir -p "$reference_dir/fingerprints"
    cp -p "$output" "$reference_dir/$image.ppm"
    statsFingerprint "output/$log" > "$reference_dir/fingerprints/$image.txt"
    touch "$manifest"
    grep -v "^$image " "$manifest" > "$manifest.tmp"
    echo "$image $(sha "$output") $(sha "$reference_dir/fingerprints/$image.txt")" >> "$manifest.tmp"
    sort "$manifest.tmp" > "$manifest"
    rm -f "$manifest.tmp"
    echo "CAPTURED: $image"
}

recordBound() {
    image="$1"
    golden="$2"
    output="output/$image.ppm"
    if [ "$golden" = "-" ]; then
        echo "SKIP: $image has no golden image"
        return
    fi
    rmse=$(runCompare RMSE "$golden_dir/$golden.ppm" "$output" null:)
    if [ $? -eq 2 ] || [ -z "$rmse" ]; then
        fail "$image: compare error against golden $golden.ppm"
        return
    fi
    touch "$bounds"
    grep -v "^$image " "$bounds" > "$bounds.tmp"
    echo "$image $golden.ppm $rmse" >> "$bounds.tmp"
    sort "$bounds.tmp" > "$bounds"
    rm -f "$bounds.tmp"
    echo "BOUND: $image $rmse"
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
    golden=$(echo "$line" | cut -d'|' -f4)
    if isOptional "$line" && ! optionalReady "$image" "$log"; then
        continue
    fi
    case "$mode" in
        exact) checkExact "$image" "$log" ;;
        golden) checkGolden "$image" "$golden" ;;
        all) checkExact "$image" "$log"; checkGolden "$image" "$golden" ;;
        capture) captureReference "$image" "$log" ;;
        bounds) recordBound "$image" "$golden" ;;
    esac
done

if [ "$failures" -eq 0 ]; then
    echo "TEST PASS"
    exit 0
fi

echo "*** TEST ERROR! *** $failures failure(s)"
exit 1
