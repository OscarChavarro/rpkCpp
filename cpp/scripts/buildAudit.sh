#!/bin/sh
# T0 build audit. Builds the tree from scratch with and without the
# raytracing modules into <work directory> and prints:
#   - every distinct compiler warning (file:line stripped of the tree prefix),
#     per configuration, in <work directory>/warnings-<configuration>.txt
#   - every class name that is defined by more than one header of the build
#     closure, in <work directory>/duplicateClasses.txt
#
#   buildAudit.sh <work directory>
#
# Compare the outputs of two trees with diff to see new warnings.

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
WORK_DIR="${1:?usage: buildAudit.sh <work directory>}"
mkdir -p "$WORK_DIR"
WORK_DIR=$(CDPATH= cd -- "$WORK_DIR" && pwd)
status=0

for configuration in raytracing noraytracing; do
    if [ "$configuration" = "raytracing" ]; then
        flag=ON
    else
        flag=OFF
    fi
    build="$WORK_DIR/$configuration"
    rm -rf "$build"
    cmake -S "$PROJECT_DIR" -B "$build/top" \
        -DWITH_RAYTRACING=$flag -DWITH_RAYCASTING=$flag \
        -DRPK_VITRAL_BINARY_DIR="$build/base" \
        -DRPK_BASE_BINARY_DIR="$build/rpkBase" \
        -DRPK_OPENGL1_BINARY_DIR="$build/rpkOpenGL1" \
        -DRPK_RENDERPARK_BINARY_DIR="$build/rpk" >/dev/null || status=1
    cmake --build "$build/top" -j "$(nproc)" >"$build.log" 2>&1 || status=1
    grep -E 'warning:' "$build.log" |
        sed -e "s|$PROJECT_DIR/||g" |
        sort -u >"$WORK_DIR/warnings-$configuration.txt"
    echo "$configuration: $(wc -l <"$WORK_DIR/warnings-$configuration.txt") distinct warnings, build log $build.log"
    if grep -q -E 'error:|Error [0-9]' "$build.log"; then
        echo "$configuration: BUILD FAILED"
        status=1
    fi
done

# Class definitions (not forward declarations) by header
find "$PROJECT_DIR/base/src/main" "$PROJECT_DIR/rpkBase/src/main" "$PROJECT_DIR/rpkOpenGL1/src" "$PROJECT_DIR/testsuite" \
    -name '*.h' -print 2>/dev/null |
    xargs grep -H -E '^(class|struct) [A-Za-z_][A-Za-z0-9_]*( final)? *(:[^;]*)?\{?$' 2>/dev/null |
    sed -e "s|$PROJECT_DIR/||" -e 's/:\(class\|struct\) \([A-Za-z_][A-Za-z0-9_]*\).*/ \2/' |
    awk '{ print $2, $1 }' | sort >"$WORK_DIR/classes.txt"
awk '{ count[$1]++; where[$1] = where[$1] " " $2 } END { for ( c in count ) if ( count[c] > 1 ) print c ":" where[c] }' \
    "$WORK_DIR/classes.txt" | sort >"$WORK_DIR/duplicateClasses.txt"
echo "duplicate class definitions: $(wc -l <"$WORK_DIR/duplicateClasses.txt") ($WORK_DIR/duplicateClasses.txt)"

exit $status
