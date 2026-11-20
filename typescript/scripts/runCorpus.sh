#!/bin/sh
# Runs the test corpus (or the images named as arguments) in parallel and waits
# for every run to finish. Each run writes output/<image>.ppm and the logs
# output/<log>.log (stdout) and output/<log>.err (stderr) used by
# testReviewResults.sh.
# Run from cpp/ as working directory.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

mkdir -p output

# A port may build once here and export settings for the runs
if [ -f "$SCRIPT_DIR/corpusPrepare.sh" ]; then
    . "$SCRIPT_DIR/corpusPrepare.sh" || exit 1
fi

selection=$(corpusSelect "$@")
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

pids=""
for line in $selection; do
    script=$(echo "$line" | cut -d'|' -f2)
    log=$(echo "$line" | cut -d'|' -f3)
    sh -c "bash '$SCRIPT_DIR/$script.sh' > 'output/$log.log' 2> 'output/$log.err'" &
    pids="$pids $!"
done

status=0
for pid in $pids; do
    if ! wait "$pid"; then
        status=1
    fi
done
exit $status
