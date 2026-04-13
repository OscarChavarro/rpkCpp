#!/bin/bash

base_image="../doc/testBaseImages/01_cube.ppm"
test_images=(
    "output/01_cubeJacobi.ppm"
    "output/01_cubeGaussSiedel.ppm"
    "output/02_cubeSouthwell.ppm"
)

if [ ! -f "$base_image" ]; then
    echo "*** TEST ERROR! ***"
    echo "Base image not found: $base_image"
    exit 1
fi

missing=0
mismatch=0

for image in "${test_images[@]}"; do
    if [ ! -f "$image" ]; then
        echo "Missing output image: $image"
        missing=1
        continue
    fi

    if cmp -s "$base_image" "$image"; then
        echo "MATCH: $image"
    else
        echo "MISMATCH: $image"
        mismatch=1
    fi
done

if [ "$missing" -eq 0 ] && [ "$mismatch" -eq 0 ]; then
    echo "TEST PASS"
    exit 0
fi

echo "*** TEST ERROR! ***"
exit 1
