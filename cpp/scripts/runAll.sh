#!/bin/bash
# Starts the test corpus (scripts/runCorpus.sh) in the background and returns
# at once; the runs go on after the terminal is closed. With no arguments it
# also starts the scenes outside the corpus (08 soda). Arguments select corpus
# entries by image or script name, as in runCorpus.sh.
# The corpus runs write output/<image>.ppm, output/<log>.log and
# output/<log>.err, the files testReviewResults.sh reads; runCorpus.sh's own
# messages and exit status go to output/runCorpus.log.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
mkdir -p output

nohup sh -c "sh '$SCRIPT_DIR/runCorpus.sh' \"\$@\"; echo \"runCorpus.sh exit status: \$?\"" runCorpus "$@" > output/runCorpus.log 2>&1 &
disown

if [ $# -eq 0 ]; then
    nohup bash "$SCRIPT_DIR/08_runSoda.sh" > output/08.log 2> output/08.err &
    disown
fi
