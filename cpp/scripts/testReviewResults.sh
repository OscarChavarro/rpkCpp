#!/bin/bash

base_dir="../doc/testBaseImages"
fallback_base_image="$base_dir/01_cube.ppm"

declare -A test_to_base=(
    ["output/01_cubeJacobi.ppm"]="$base_dir/01_cubeJacobi.ppm"
    ["output/01_cubeGaussSiedel.ppm"]="$base_dir/01_cubeGaussSiedel.ppm"
    ["output/01_cubeSouthwell.ppm"]="$base_dir/01_cubeSouthwell.ppm"
)

if [ "${1:-}" = "--refresh-baseline" ]; then
    mkdir -p "$base_dir"
    for image in "${!test_to_base[@]}"; do
        if [ ! -f "$image" ]; then
            echo "*** TEST ERROR! ***"
            echo "Missing output image to refresh baseline: $image"
            exit 1
        fi
        cp "$image" "${test_to_base[$image]}"
        echo "UPDATED BASELINE: ${test_to_base[$image]}"
    done
    exit 0
fi

missing=0
mismatch=0

for image in "${!test_to_base[@]}"; do
    base_image="${test_to_base[$image]}"
    if [ ! -f "$base_image" ]; then
        if [ -f "$fallback_base_image" ]; then
            base_image="$fallback_base_image"
        else
            echo "*** TEST ERROR! ***"
            echo "Base image not found for $image: ${test_to_base[$image]}"
            missing=1
            continue
        fi
    fi

    if [ ! -f "$image" ]; then
        echo "Missing output image: $image"
        missing=1
        continue
    fi

    if cmp -s "$base_image" "$image"; then
        echo "MATCH: $image (base: $base_image)"
    else
        echo "MISMATCH: $image (base: $base_image)"
        mismatch=1
    fi
done

if [ "$missing" -eq 0 ] && [ "$mismatch" -eq 0 ]; then
    echo "TEST PASS"
    exit 0
fi

echo "*** TEST ERROR! ***"
exit 1
