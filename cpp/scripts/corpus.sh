#!/bin/sh
# Test corpus table shared by runCorpus.sh and testReviewResults.sh.
# One line per output image: image name | run script | log name. The golden
# image of each output is etc/goldenImagesForTests/<image name>.ppm.

CORPUS_TABLE='01_cubeJacobi|01_runCubeJacobi|01_jacobi
01_cubeGaussSiedel|01_runCubeGaussSiedel|01_gaussSiedel
01_cubeSouthwell|01_runCubeSouthwell|01_southwell
02_corridor|02_runCorridor|02
03_hospital|03_runHospital|03
04_office1|04_runOffice1|04
05_office2|05_runOffice2|05
06_office3|06_runOffice3|06
07_classroom|07_runSalonDeClase|07
10_floorRayMatting|10_floorRayMatting|10
11_floorRayCasting|11_floorRayCasting|11
12_floorBidirectionalPathTracing|12_floorBidirectionalPathTracing|12
13_floorStochasticRaytracingRandomWalk|13_floorStochasticRaytracingRandomWalk|13
14_floorStochasticRaytracingJacobi|14_floorStochasticRaytracingJacobi|14
15_cubeJacobiImportance|15_runCubeJacobiImportance|15
17_floorStochasticJacobiImportance|17_floorStochasticJacobiImportance|17
21_tonemapWard|21_tonemap_Ward|21
22_tonemapTumblinRushmeier|22_tonemap_TumblinRushmeier|22'

# Scenes outside the corpus, with the same columns. scripts/runAll.sh starts
# them beside the corpus and does not wait for them, so
# testReviewResults.sh compares one only when its run has finished, that is
# when output/<log>.status (the run's exit status, written at its end) exists,
# and otherwise reports that it was not compared.
OPTIONAL_TABLE='08_soda|08_runSoda|08'

# Runs one corpus script from cpp/: stdout to output/<log>.log, stderr to
# output/<log>.err, the CPU time of the run (the script and the rpk process
# it waits for: user and system seconds), its wall clock and peak RSS to
# output/<log>.time when GNU time is installed, and at the end its exit
# status to output/<log>.status. Used by runCorpus.sh and runAll.sh.
corpusRunEntry() {
    script_dir="$1"
    script="$2"
    log="$3"
    if [ -x /usr/bin/time ]; then
        /usr/bin/time -o "output/$log.time" -f 'user %U
system %S
elapsed %e
maxRssKb %M' bash "$script_dir/$script.sh" > "output/$log.log" 2> "output/$log.err"
    else
        bash "$script_dir/$script.sh" > "output/$log.log" 2> "output/$log.err"
    fi
    status=$?
    echo "$status" > "output/$log.status"
    return $status
}

# Selects the corpus lines whose image name is in the argument list (all when empty)
corpusSelect() {
    if [ $# -eq 0 ]; then
        echo "$CORPUS_TABLE"
        return
    fi
    for wanted in "$@"; do
        printf '%s\n%s\n' "$CORPUS_TABLE" "$OPTIONAL_TABLE" | awk -F'|' -v w="$wanted" '$1 == w || $2 == w { print }'
    done
}

# Selects the scenes outside the corpus named in the argument list (all when empty)
optionalSelect() {
    if [ $# -eq 0 ]; then
        echo "$OPTIONAL_TABLE"
        return
    fi
    for wanted in "$@"; do
        echo "$OPTIONAL_TABLE" | awk -F'|' -v w="$wanted" '$1 == w || $2 == w { print }'
    done
}

# True when the line is a scene outside the corpus
isOptional() {
    echo "$OPTIONAL_TABLE" | grep -qxF "$1"
}
