#!/bin/bash
# Starts the test corpus (scripts/runCorpus.sh) in the background and returns
# at once; the runs go on after the terminal is closed. With no arguments it
# also starts the scenes outside the corpus (OPTIONAL_TABLE in corpus.sh: 08
# soda), each writing output/<log>.status when it ends. Arguments select corpus
# entries by image or script name, as in runCorpus.sh.
# The corpus runs write output/<image>.png, output/<log>.log and
# output/<log>.err, the files testReviewResults.sh reads; runCorpus.sh's own
# messages and exit status go to output/runCorpus.log.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
mkdir -p output

# A port that builds before its runs (corpusPrepare.sh) builds once here, for
# the corpus and for the scenes outside it, which start at the same time
if [ -f "$SCRIPT_DIR/corpusPrepare.sh" ]; then
    . "$SCRIPT_DIR/corpusPrepare.sh" || exit 1
    export RPK_CORPUS_PREPARED=1
fi

nohup sh -c "sh '$SCRIPT_DIR/runCorpus.sh' \"\$@\"; echo \"runCorpus.sh exit status: \$?\"" runCorpus "$@" > output/runCorpus.log 2>&1 &
disown

if [ $# -eq 0 ]; then
    . "$SCRIPT_DIR/corpus.sh"
    for line in $OPTIONAL_TABLE; do
        image=$(echo "$line" | cut -d'|' -f1)
        script=$(echo "$line" | cut -d'|' -f2)
        log=$(echo "$line" | cut -d'|' -f3)
        # A run still in progress, or one that crashed, must not leave the
        # previous run's results to be compared
        rm -f "output/$image.png" "output/$log.log" "output/$log.err" "output/$log.status" "output/$log.time"
        nohup bash -c ". '$SCRIPT_DIR/corpus.sh'; corpusRunEntry '$SCRIPT_DIR' '$script' '$log'" > /dev/null 2>&1 &
        disown
    done
fi
