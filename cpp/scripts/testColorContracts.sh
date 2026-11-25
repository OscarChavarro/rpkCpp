#!/bin/sh
# Run after scripts/compile.sh with ray tracing enabled.
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/rpk-color-contracts.XXXXXXXX")
trap 'rm -f "$test_dir/screen" "$test_dir/cluster" "$test_dir/basis"; rmdir "$test_dir"' EXIT

for name in screen cluster basis; do
    case "$name" in
        screen) source_file=ScreenBufferColorContract.cpp ;;
        cluster) source_file=ClusterColorContract.cpp ;;
        basis) source_file=StochasticBasisColorContract.cpp ;;
    esac
    "${CXX:-c++}" -std=c++11 -DRAYTRACING_ENABLED=true \
        -I "$project_dir/rpkBase/src/main" -I "$project_dir/base/src/main" \
        "$project_dir/testsuite/ContractCases/$source_file" \
        -L "$project_dir/rpkBase/build" -L "$project_dir/base/build" \
        -Wl,-rpath,"$project_dir/rpkBase/build:$project_dir/base/build" \
        -lrpkBase -lvitral -o "$test_dir/$name"
    "$test_dir/$name"
done
echo "Color contracts passed"
