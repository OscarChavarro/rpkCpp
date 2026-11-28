#!/bin/bash
# Z-visibility variant of the Galerkin scenes of the test corpus:
#
#   runZVisibility.sh [--serial | --threads N] [--reference DIR] [image or script name ...]
#
# Runs every Galerkin scene of the corpus (or the ones named) as its script
# gives it, with -gr-clustering-strategy z-visibility appended, so that the
# cluster interactions are evaluated with the scratch visibility buffer
# (ScratchVisibilityStrategy and the render/raster/polygonScanConverter rasterizer) instead
# of the default isotropic clustering. The scenes run one after the other, as
# in runAllParallel.sh, and each one uses the machine in parallel: the runs
# that iterate with Jacobi run with -gr-iteration-method parallel-jacobi
# -gr-threads N, N being --threads, $RPK_THREADS or, by default, the number of
# online processors (nproc); Gauss-Seidel and Southwell runs are unchanged.
# Parallel Jacobi gives the serial result bit for bit, Z-visibility included.
# With --serial the Jacobi runs are serial (-gr-iteration-method jacobi).
#
# Outputs and logs go to output/zVisibility/ (output/zVisibilitySerial/ with
# --serial). These images have no golden images: a scene passes when its run
# succeeds and writes its image. With --reference DIR each image must also
# equal DIR/<image>.png (compare -metric AE = 0), for example the outputs of
# a --serial run, or of another port. As a plain indication, the RMSE against
# the golden image of the isotropic run, etc/goldenImagesForTests/<image>.png,
# is printed too. The scenes outside the corpus (08 soda) run only when named.
# The exit status is 0 only when every run succeeded and every image matched.
#
# TypeScript: the scene scripts run the program themselves (runNodeProgram.mjs)
# and pass their own arguments on to it, so instead of the C++ program wrapper
# the options are appended to the scene script's command line (the last one
# wins): the clustering strategy, the Jacobi options and the image file, saved
# to the output directory. The program is built once, before the first scene.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
. "$SCRIPT_DIR/corpus.sh"

threads="${RPK_THREADS:-$(nproc)}"
serial=0
reference_dir=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --serial) serial=1; shift ;;
        --threads)
            threads="${2:-}"
            shift 2 ;;
        --reference)
            reference_dir="${2:-}"
            if [ ! -d "$reference_dir" ]; then
                echo "Reference directory not found: $reference_dir"
                exit 2
            fi
            shift 2 ;;
        --) shift; break ;;
        --*) echo "Unknown option $1"; exit 2 ;;
        *) break ;;
    esac
done
case "$threads" in ''|*[!0-9]*|0) echo "Threads must be a positive integer"; exit 2 ;; esac

if ! command -v compare >/dev/null 2>&1; then
    echo "*** TEST ERROR! *** ImageMagick 'compare' is not installed"
    exit 2
fi

output_dir="output/zVisibility"
if [ $serial -eq 1 ]; then
    output_dir="output/zVisibilitySerial"
fi
golden_dir="etc/goldenImagesForTests"
mkdir -p "$output_dir"

if [ $serial -eq 1 ]; then
    jacobi_options="-gr-iteration-method jacobi"
else
    jacobi_options="-gr-iteration-method parallel-jacobi -gr-threads $threads"
fi

# Build once; the scene scripts then skip their own build
if [ -z "${RPK_SKIP_BUILD:-}" ]; then
    npm run build >/dev/null || exit 1
    export RPK_SKIP_BUILD=1
fi

# The Galerkin scenes of the corpus, or the ones named among them
selection=$(corpusSelect "$@" | while IFS='|' read -r image script log; do
    if [ "$#" -eq 0 ] && isOptional "$image|$script|$log"; then
        continue
    fi
    if grep -q -- '-radiance-method Galerkin' "$SCRIPT_DIR/$script.sh"; then
        echo "$image|$script|$log"
    fi
done)
if [ -z "$selection" ]; then
    echo "No Galerkin corpus scene matches: $*"
    exit 2
fi

failures=0
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    script=$(echo "$line" | cut -d'|' -f2)
    log=$(echo "$line" | cut -d'|' -f3)

    # The scene iterates with Jacobi when its script says so, or says nothing
    # (the default iteration method is Jacobi)
    method=$(sed -n 's/.*-gr-iteration-method \([^ ]*\).*/\1/p' "$SCRIPT_DIR/$script.sh" | tail -1)
    extra=(-gr-clustering-strategy z-visibility)
    case "$method" in
        ''|ja*|JA*|Ja*|parallel-jacobi) extra+=($jacobi_options) ;;
    esac
    extra+=(-radiance-image-savefile "./$output_dir/$image.png")

    # A run that crashes must not leave the previous run's results to be compared
    rm -f "$output_dir/$image.png" "$output_dir/$log.log" "$output_dir/$log.err" "$output_dir/$log.time"
    start=$(date +%s.%N)
    if [ -x /usr/bin/time ]; then
        /usr/bin/time -o "$output_dir/$log.time" -f 'user %U
system %S
elapsed %e
maxRssKb %M' bash "$SCRIPT_DIR/$script.sh" "${extra[@]}" > "$output_dir/$log.log" 2> "$output_dir/$log.err"
    else
        bash "$SCRIPT_DIR/$script.sh" "${extra[@]}" > "$output_dir/$log.log" 2> "$output_dir/$log.err"
    fi
    status=$?
    end=$(date +%s.%N)
    seconds=$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.2f", e - s }')

    if [ $status -ne 0 ] || [ ! -f "$output_dir/$image.png" ]; then
        printf '%-28s FAILED RUN (status %d) %8s s\n' "$image" "$status" "$seconds"
        failures=$((failures + 1))
        continue
    fi
    isotropic="-"
    if [ -f "$golden_dir/$image.png" ]; then
        isotropic=$(compare -metric RMSE "$output_dir/$image.png" "$golden_dir/$image.png" null: 2>&1 |
            sed -n 's/.*(\(.*\)).*/\1/p')
    fi
    if [ -z "$reference_dir" ]; then
        printf '%-28s DONE %8s s  (RMSE vs isotropic %s)\n' "$image" "$seconds" "$isotropic"
        continue
    fi
    if [ ! -f "$reference_dir/$image.png" ]; then
        printf '%-28s NO REFERENCE IMAGE %8s s\n' "$image" "$seconds"
        failures=$((failures + 1))
        continue
    fi
    difference=$(compare -metric AE "$output_dir/$image.png" "$reference_dir/$image.png" null: 2>&1)
    if [ "$difference" = "0" ]; then
        printf '%-28s MATCH %8s s  (RMSE vs isotropic %s)\n' "$image" "$seconds" "$isotropic"
    else
        printf '%-28s MISMATCH AE %s %8s s\n' "$image" "$difference" "$seconds"
        failures=$((failures + 1))
    fi
done

if [ $failures -ne 0 ]; then
    echo "TEST FAIL: $failures scene(s)"
    exit 1
fi
echo "TEST PASS"
exit 0
