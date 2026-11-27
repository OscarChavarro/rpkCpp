#!/bin/bash
# Z-visibility variant of the Galerkin scenes of the test corpus:
#
#   runZVisibility.sh [--reference DIR] [image or script name ...]
#
# Runs every Galerkin scene of the corpus (or the ones named) as its script
# gives it, with -gr-clustering-strategy z-visibility appended, so that the
# cluster interactions are evaluated with the scratch visibility buffer
# (ScratchVisibilityStrategy and the render/raster/polygonScanConverter rasterizer) instead
# of the default isotropic clustering. The scenes run one after the other.
# Turbo C++ port note: this port has no parallel Jacobi, so the cpp port's
# --serial and --threads options are not offered: every run is serial, as
# the cpp port's runs with --serial (which give its parallel results bit for
# bit).
#
# Outputs and logs go to output/zVisibility/. These images have no golden
# images: a scene passes when its run succeeds and writes its image. With
# --reference DIR each image must also equal DIR/<image>.ppm (compare -metric
# AE = 0), for example the outputs of the cpp port's runZVisibility.sh, or of
# another build given with RPK_BIN. As a plain indication, the RMSE against the
# reference image of the isotropic run, etc/goldenImagesForTests/<image>.ppm,
# is printed too. The exit status is 0 only when every run succeeded and every
# image matched.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
. "$SCRIPT_DIR/corpus.sh"

reference_dir=""
while [ "$#" -gt 0 ]; do
    case "$1" in
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

if ! command -v compare >/dev/null 2>&1; then
    echo "*** TEST ERROR! *** ImageMagick 'compare' is not installed"
    exit 2
fi

output_dir="output/zVisibility"
golden_dir="etc/goldenImagesForTests"
mkdir -p "$output_dir"

program="${RPK_BIN:-$(pwd)/testsuite/ApplicationCases/RenderparkApplication/build/rpk}"

# The program as seen by the scene scripts: the real one, with the
# z-visibility clustering strategy appended and the image saved to
# output_dir instead of output/
wrapper="$(pwd)/$output_dir/zVisibilityRpk.sh"
cat > "$wrapper" <<WRAPPER
#!/bin/bash
arguments=()
previous=""
for argument in "\$@"; do
    if [ "\$previous" = "-radiance-image-savefile" ] || [ "\$previous" = "-raytracing-image-savefile" ]; then
        argument="$output_dir/\$(basename "\$argument")"
    fi
    arguments+=("\$argument")
    previous="\$argument"
done
arguments+=(-gr-clustering-strategy z-visibility)
exec "$program" "\${arguments[@]}"
WRAPPER
chmod +x "$wrapper"
export RPK_BIN="$wrapper"

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
    # A run that crashes must not leave the previous run's results to be compared
    rm -f "$output_dir/$image.ppm" "$output_dir/$log.log" "$output_dir/$log.err" "$output_dir/$log.time"
    start=$(date +%s.%N)
    if [ -x /usr/bin/time ]; then
        /usr/bin/time -o "$output_dir/$log.time" -f 'user %U
system %S
elapsed %e
maxRssKb %M' bash "$SCRIPT_DIR/$script.sh" > "$output_dir/$log.log" 2> "$output_dir/$log.err"
    else
        bash "$SCRIPT_DIR/$script.sh" > "$output_dir/$log.log" 2> "$output_dir/$log.err"
    fi
    status=$?
    end=$(date +%s.%N)
    seconds=$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.2f", e - s }')

    if [ $status -ne 0 ] || [ ! -f "$output_dir/$image.ppm" ]; then
        printf '%-28s FAILED RUN (status %d) %8s s\n' "$image" "$status" "$seconds"
        failures=$((failures + 1))
        continue
    fi
    isotropic="-"
    if [ -f "$golden_dir/$image.ppm" ]; then
        isotropic=$(compare -metric RMSE "$output_dir/$image.ppm" "$golden_dir/$image.ppm" null: 2>&1 |
            sed -n 's/.*(\(.*\)).*/\1/p')
    fi
    if [ -z "$reference_dir" ]; then
        printf '%-28s DONE %8s s  (RMSE vs isotropic %s)\n' "$image" "$seconds" "$isotropic"
        continue
    fi
    if [ ! -f "$reference_dir/$image.ppm" ]; then
        printf '%-28s NO REFERENCE IMAGE %8s s\n' "$image" "$seconds"
        failures=$((failures + 1))
        continue
    fi
    difference=$(compare -metric AE "$output_dir/$image.ppm" "$reference_dir/$image.ppm" null: 2>&1)
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
