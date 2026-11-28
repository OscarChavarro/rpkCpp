#!/bin/sh
# Runs the test corpus (or the images named as arguments) in parallel and waits
# for every run to finish. Each run writes output/<image>.png and the logs
# output/<log>.log (stdout) and output/<log>.err (stderr) used by
# testReviewResults.sh, output/<log>.time (CPU time, wall clock and peak RSS,
# read by timeReport.sh) and at its end output/<log>.status (exit status).
# Run from java/ as working directory.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

mkdir -p output

# A port may build once here and export settings for the runs
# (runAll.sh does it once for the corpus and the scenes outside it)
if [ -f "$SCRIPT_DIR/corpusPrepare.sh" ] && [ -z "${RPK_CORPUS_PREPARED:-}" ]; then
    . "$SCRIPT_DIR/corpusPrepare.sh" || exit 1
fi

selection=$(corpusSelect "$@")
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

pids=""
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    script=$(echo "$line" | cut -d'|' -f2)
    log=$(echo "$line" | cut -d'|' -f3)
    # A run that crashes must not leave the previous run's image to be compared
    rm -f "output/$image.png" "output/$log.log" "output/$log.err" "output/$log.status" "output/$log.time"
    corpusRunEntry "$SCRIPT_DIR" "$script" "$log" &
    pids="$pids $!"
done

status=0
for pid in $pids; do
    if ! wait "$pid"; then
        status=1
    fi
done
exit $status
