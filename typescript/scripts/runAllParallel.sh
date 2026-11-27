#!/bin/bash
# Runs the test scenes one after the other, in the order of their number, and
# returns when the last one ends, so the whole suite can be timed:
#   time scripts/runAllParallel.sh
# Each run is blocking, but uses the machine in parallel: the Galerkin runs
# that iterate with Jacobi (given with -gr-iteration-method jacobi, or by
# default) run with -gr-iteration-method parallel-jacobi -gr-threads N, N being
# $RPK_THREADS or, by default, 4. Gauss-Seidel and Southwell runs, and the
# runs of other methods, are unchanged.
# Parallel Jacobi gives the serial result bit for bit, so the outputs, in
# output/ as with runAll.sh, are checked against the same references
# (scripts/testReviewResults.sh).
# Options: --threads N and --cpu-groups "LIST;LIST" (round-robin per scene).
# Each LIST is a taskset CPU list, for example "0-17;18-35".
# Arguments select corpus entries by image or script name, as in runCorpus.sh;
# with none, every scene runs, the ones outside the corpus (08 soda) included.
# The exit status is 0 only when every run succeeded.
#
# TypeScript: every parallel Jacobi thread is a replica of the program, with
# its own copy of the scene (see RefinementReplicas), so the default number of
# threads is not the number of online processors (as in C++) but a small one,
# whose memory a run fits in. Parallel Random Walk final tracing is not ported.
# The scene scripts run "node scripts/runNodeProgram.mjs ... -- <rpk arguments>":
# the program they see is a "node" first in PATH that appends the parallel
# Jacobi options to those runs (the last option wins) and runs the real node.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
mkdir -p output

threads="${RPK_THREADS:-4}"
. "$SCRIPT_DIR/../../scripts/parallelAffinity.sh"
rpk_parallel_options "$@" || exit $?
set -- "${RPK_PARALLEL_ARGS[@]}"
node="$(command -v node)"
if [ -z "$node" ]; then
    echo "node is not installed"
    exit 2
fi

wrapperDir="$(pwd)/output/parallelJacobiBin"
mkdir -p "$wrapperDir"
wrapper="$wrapperDir/node"
cat > "$wrapper" <<EOF
#!/bin/bash
. "$SCRIPT_DIR/../../scripts/parallelAffinity.sh"
case "\$1" in
    *runNodeProgram.mjs) ;;
    *) exec "$node" "\$@" ;;
esac
galerkin=0
jacobi=1
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
    previous="\$argument"
done
if [ \$galerkin -eq 1 ] && [ \$jacobi -eq 1 ]; then
    rpk_parallel_exec "$node" "\$@" -gr-iteration-method parallel-jacobi -gr-threads $threads
fi
rpk_parallel_exec "$node" "\$@"
EOF
chmod +x "$wrapper"

export PATH="$wrapperDir:$PATH"

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
