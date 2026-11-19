#!/bin/sh

base_dir="../doc/testBaseImages"
fallback_base_image="$base_dir/01_cube.ppm"

if [ "${1:-}" = "--refresh-baseline" ]; then
    mkdir -p "$base_dir"
fi

missing=0
mismatch=0

check_image() {
    image="$1"
    requested_base="$2"
    base_image="$requested_base"

    if [ ! -f "$base_image" ]; then
        if [ -f "$fallback_base_image" ]; then
            base_image="$fallback_base_image"
        else
            echo "*** TEST ERROR! ***"
            echo "Base image not found for $image: $requested_base"
            missing=1
            return
        fi
    fi

    if [ ! -f "$image" ]; then
        echo "Missing output image: $image"
        missing=1
        return
    fi

    if findimagedupes -t 100 "$base_image" "$image" 2>/dev/null | grep -q .; then
        echo "MATCH: $image (base: $base_image)"
    else
        echo "MISMATCH: $image (base: $base_image)"
        mismatch=1
    fi
}

check_image "output/01_cubeJacobi.ppm" "$base_dir/01_cube.ppm"
check_image "output/01_cubeGaussSiedel.ppm" "$base_dir/01_cube.ppm"
check_image "output/01_cubeSouthwell.ppm" "$base_dir/01_cubeSouthwell.ppm"
check_image "output/02_corridor.ppm" "$base_dir/02_corridor.ppm"
check_image "output/03_hospital.ppm" "$base_dir/03_hospital.ppm"
check_image "output/04_office1.ppm" "$base_dir/04_office1.ppm"
check_image "output/05_office2.ppm" "$base_dir/05_office2.ppm"
check_image "output/06_office3.ppm" "$base_dir/06_office3.ppm"
check_image "output/07_classroom.ppm" "$base_dir/07_classroom.ppm"
check_image "output/08_soda.ppm" "$base_dir/08_soda.ppm"
check_image "output/10_floorRayMatting.ppm" "$base_dir/10_floorRayMatting.ppm"
check_image "output/11_floorRayCasting.ppm" "$base_dir/11_floorRayCasting.ppm"
check_image "output/12_floorBidirectionalPathTracing.ppm" "$base_dir/12_floorBidirectionalPathTracing.ppm"
check_image "output/13_floorStochasticRaytracingRandomWalk.ppm" "$base_dir/13_floorStochasticRaytracingRandomWalk.ppm"
check_image "output/14_floorStochasticRaytracingJacobi.ppm" "$base_dir/14_floorStochasticRaytracingJacobi.ppm"
check_image "output/21_tonemapWard.ppm" "$base_dir/21_tonemapWard.ppm"
check_image "output/22_tonemapTumblinRushmeier.ppm" "$base_dir/22_tonemapTumblinRushmeier.ppm"

if [ "$missing" -eq 0 ] && [ "$mismatch" -eq 0 ]; then
    echo "TEST PASS"
    exit 0
fi

echo "*** TEST ERROR! ***"
exit 1
