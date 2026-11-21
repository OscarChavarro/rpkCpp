#!/bin/sh
# Sourced by runCorpus.sh: build once, then let the parallel runs skip their own build.
npm run build >/dev/null || return 1
export RPK_SKIP_BUILD=1
