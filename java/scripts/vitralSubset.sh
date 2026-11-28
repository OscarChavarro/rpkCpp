#!/bin/sh
# Checks or refreshes the VITRAL Java source subsets: the files listed in
# vitralSubset.txt (base/src/main, from VITRAL's java/base/src/main) and in
# vitralAwtSubset.txt (awt/src/main, from VITRAL's java/awt/src/main).
#
#   vitralSubset.sh --check   <VITRAL checkout>   fails if any listed file differs or is missing,
#                                                 or if base/src/main or awt/src/main holds an
#                                                 unlisted file
#   vitralSubset.sh --refresh <VITRAL checkout>   copies the listed files from VITRAL, then checks
#
# Paths in each list are relative to the module's src/main in both Java trees.
# The refresh writes only into this port's base/src/main and awt/src/main.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
MODE="${1:?usage: vitralSubset.sh --check|--refresh <VITRAL checkout>}"
VITRAL_JAVA="${2:?usage: vitralSubset.sh --check|--refresh <VITRAL checkout>}/java"

if [ "$MODE" != "--refresh" ] && [ "$MODE" != "--check" ]; then
    echo "Unknown mode $MODE"
    exit 2
fi

status=0

# checkModule <module folder> <list file>
checkModule() {
    RPK_MAIN="$SCRIPT_DIR/../$1/src/main"
    VITRAL_MAIN="$VITRAL_JAVA/$1/src/main"
    LIST="$SCRIPT_DIR/$2"

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
    fi

    moduleStatus=0
    count=0
    for f in $files; do
        count=$((count + 1))
        if [ ! -f "$VITRAL_MAIN/$f" ]; then
            echo "MISSING IN VITRAL: $f"
            moduleStatus=1
        elif [ ! -f "$RPK_MAIN/$f" ]; then
            echo "MISSING IN RPK: $f"
            moduleStatus=1
        elif ! cmp -s "$VITRAL_MAIN/$f" "$RPK_MAIN/$f"; then
            echo "DIFFERS: $f"
            diff "$VITRAL_MAIN/$f" "$RPK_MAIN/$f" | head -10
            moduleStatus=1
        fi
    done

    # The module is the VITRAL copy only: any other file there belongs in rpkBase/
    extra=$(cd "$RPK_MAIN" && find . -type f | sed 's#^\./##' | sort | while read -r f; do
        printf '%s\n' "$files" | grep -qxF "$f" || echo "$f"
    done)
    for f in $extra; do
        echo "NOT IN SUBSET (move it to rpkBase/ or list it): $1/$f"
        moduleStatus=1
    done

    if [ $moduleStatus -eq 0 ]; then
        echo "vitralSubset: $count files identical to $VITRAL_MAIN"
    else
        status=1
    fi
}

checkModule base vitralSubset.txt
checkModule awt vitralAwtSubset.txt
exit $status
