#!/bin/bash
# Runs the test scenes one after the other, in the order of their number, and
# returns when the last one ends, so the whole suite can be timed:
#   time scripts/runAllParallel.sh
# Each run is blocking, but uses the machine in parallel: the Galerkin runs
# that iterate with Jacobi (given with -gr-iteration-method jacobi, or by
# default) run with -gr-iteration-method parallel-jacobi -gr-threads N, N being
# $RPK_THREADS or, by default, the number of online processors (nproc).
# Gauss-Seidel and Southwell runs are unchanged. Random Walk with
# StochasticRaytracing receives the same worker count for its final image.
# Parallel Jacobi gives the serial result bit for bit, so the outputs, in
# output/ as with runAll.sh, are checked against the same references
# (scripts/testReviewResults.sh). Parallel Random Walk uses a different random
# sequence from the serial reference.
# Options: --threads N and --cpu-groups "LIST;LIST" (round-robin per scene).
# Each LIST is a taskset CPU list, for example "0-17;18-35".
# Arguments select corpus entries by image or script name, as in runCorpus.sh;
# with none, every scene runs, the ones outside the corpus (08 soda) included.
# The exit status is 0 only when every run succeeded.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
mkdir -p output

threads="${RPK_THREADS:-$(nproc)}"
. "$SCRIPT_DIR/../../scripts/parallelAffinity.sh"
rpk_parallel_options "$@" || exit $?
set -- "${RPK_PARALLEL_ARGS[@]}"
program="${RPK_BIN:-$(pwd)/testsuite/ApplicationCases/RenderparkApplication/build/rpk}"

# The program as seen by the scene scripts: the real one, with the parallel
# parallel options appended to eligible runs (the last option wins)
wrapper="$(pwd)/output/parallelJacobiRpk.sh"
cat > "$wrapper" <<EOF
#!/bin/bash
. "$SCRIPT_DIR/../../scripts/parallelAffinity.sh"
galerkin=0
jacobi=1
random_walk=0
stochastic=0
previous=""
for argument in "\$@"; do
    if [ "\$previous" = "-radiance-method" ] && [ "\$argument" = "Galerkin" ]; then
        galerkin=1
    fi
    if [ "\$previous" = "-gr-iteration-method" ]; then
        case "\$argument" in
            ja*|JA*|Ja*|parallel-jacobi) jacobi=1 ;;
            *) jacobi=0 ;;
        esac
    fi
    if [ "\$previous" = "-radiance-method" ] && [ "\$argument" = "RandomWalk" ]; then
        random_walk=1
    fi
    if [ "\$previous" = "-raytracing-method" ] && [ "\$argument" = "StochasticRaytracing" ]; then
        stochastic=1
    fi
    previous="\$argument"
done
if [ \$galerkin -eq 1 ] && [ \$jacobi -eq 1 ]; then
    rpk_parallel_exec "$program" "\$@" -gr-iteration-method parallel-jacobi -gr-threads $threads
fi
if [ \$random_walk -eq 1 ] && [ \$stochastic -eq 1 ]; then
    rpk_parallel_exec "$program" "\$@" -gr-threads $threads
fi
rpk_parallel_exec "$program" "\$@"
EOF
chmod +x "$wrapper"

export RPK_BIN="$wrapper"

. "$SCRIPT_DIR/corpus.sh"
if [ -f "$SCRIPT_DIR/corpusPrepare.sh" ]; then
    . "$SCRIPT_DIR/corpusPrepare.sh" || exit 1
fi

# Every scene in the order of its number: the corpus table and the scenes
# outside it, or the ones named as arguments, kept in that same order
all_scenes=$(printf '%s\n%s\n' "$CORPUS_TABLE" "$OPTIONAL_TABLE" | sort -s -t'_' -k1,1n)
if [ $# -eq 0 ]; then
    selection="$all_scenes"
else
    selection=$(echo "$all_scenes" | awk -F'|' -v names="$*" '
        BEGIN { n = split(names, wanted, " ") }
        { for ( i = 1; i <= n; i++ ) if ( $1 == wanted[i] || $2 == wanted[i] ) { print; next } }')
fi
if [ -z "$selection" ]; then
    echo "No scene matches: $*"
    exit 2
fi

failures=0
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    script=$(echo "$line" | cut -d'|' -f2)
    log=$(echo "$line" | cut -d'|' -f3)
    # A run that crashes must not leave the previous run's results to be compared
    rm -f "output/$image.ppm" "output/$log.log" "output/$log.err" "output/$log.status" "output/$log.time"
    start=$(date +%s.%N)
    corpusRunEntry "$SCRIPT_DIR" "$script" "$log"
    status=$?
    end=$(date +%s.%N)
    printf '%-40s status %d  %8.2f s\n' "$image" "$status" "$(awk -v s="$start" -v e="$end" 'BEGIN { print e - s }')"
    if [ $status -ne 0 ]; then
        failures=$((failures + 1))
    fi
done

if [ $failures -ne 0 ]; then
    echo "$failures run(s) failed"
    exit 1
fi
exit 0
