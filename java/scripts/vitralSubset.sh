#!/bin/sh
# Checks or refreshes the VITRAL Java source subset listed in vitralSubset.txt.
#
#   vitralSubset.sh --check   <VITRAL checkout>   fails if any listed file differs or is missing,
#                                                 or if base/src/main holds an unlisted file
#   vitralSubset.sh --refresh <VITRAL checkout>   copies the listed files from VITRAL, then checks
#
# Paths in the list are relative to base/src/main in both Java trees
# (java/base/src/main here, java/base/src/main in the VITRAL checkout). The
# refresh writes only into this port's java/base/src/main.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
RPK_MAIN="$SCRIPT_DIR/../base/src/main"
LIST="$SCRIPT_DIR/vitralSubset.txt"
MODE="${1:?usage: vitralSubset.sh --check|--refresh <VITRAL checkout>}"
VITRAL_MAIN="${2:?usage: vitralSubset.sh --check|--refresh <VITRAL checkout>}/java/base/src/main"

if [ ! -d "$VITRAL_MAIN" ]; then
    echo "No VITRAL Java sources at $VITRAL_MAIN"
    exit 2
fi

files=$(grep -v '^#' "$LIST" | grep -v '^$')

if [ "$MODE" = "--refresh" ]; then
    for f in $files; do
        mkdir -p "$(dirname "$RPK_MAIN/$f")"
        cp "$VITRAL_MAIN/$f" "$RPK_MAIN/$f" || exit 1
    done
elif [ "$MODE" != "--check" ]; then
    echo "Unknown mode $MODE"
    exit 2
fi

status=0
count=0
for f in $files; do
    count=$((count + 1))
    if [ ! -f "$VITRAL_MAIN/$f" ]; then
        echo "MISSING IN VITRAL: $f"
        status=1
    elif [ ! -f "$RPK_MAIN/$f" ]; then
        echo "MISSING IN RPK: $f"
        status=1
    elif ! cmp -s "$VITRAL_MAIN/$f" "$RPK_MAIN/$f"; then
        echo "DIFFERS: $f"
        diff "$VITRAL_MAIN/$f" "$RPK_MAIN/$f" | head -10
        status=1
    fi
done

# base/ is the VITRAL copy only: any other file there belongs in rpkBase/
extra=$(cd "$RPK_MAIN" && find . -type f | sed 's#^\./##' | sort | while read -r f; do
    printf '%s\n' "$files" | grep -qxF "$f" || echo "$f"
done)
for f in $extra; do
    echo "NOT IN SUBSET (move it to rpkBase/ or list it): $f"
    status=1
done

if [ $status -eq 0 ]; then
    echo "vitralSubset: $count files identical to $VITRAL_MAIN"
fi
exit $status
