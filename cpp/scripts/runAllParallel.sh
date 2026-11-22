#!/bin/bash
# runAll.sh with parallel Jacobi and parallel Random Walk final tracing.
# The Galerkin runs that iterate with Jacobi
# (given with -gr-iteration-method jacobi, or by default) run with
# -gr-iteration-method parallel-jacobi -gr-threads N, N being $RPK_THREADS or,
# by default, the number of online processors (nproc). Gauss-Seidel and
# Southwell runs are unchanged. Random Walk with StochasticRaytracing receives
# the same worker count for its final image.
# Parallel Jacobi gives the serial result bit for bit, so the outputs, in
# output/ as with runAll.sh, are checked against the same references. Parallel
# Random Walk uses a different random sequence from the serial reference:
#   scripts/runAllParallel.sh; (wait for output/runCorpus.log and 08.status)
#   scripts/testReviewResults.sh
# Options: --threads N and --cpu-groups "LIST;LIST" (round-robin per scene).
# Each LIST is a taskset CPU list, for example "0-17;18-35".
# Arguments select corpus entries as in runAll.sh (soda runs only with none).

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
exec bash "$SCRIPT_DIR/runAll.sh" "$@"
