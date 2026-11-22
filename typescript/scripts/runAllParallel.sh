#!/bin/bash
# runAll.sh with parallel Jacobi: the Galerkin runs that iterate with Jacobi
# (given with -gr-iteration-method jacobi, or by default) run with
# -gr-iteration-method parallel-jacobi -gr-threads N, N being $RPK_THREADS or,
# by default, 4. Gauss-Seidel and Southwell runs, and the runs of other
# methods, are unchanged.
# Parallel Jacobi gives the serial result bit for bit, so the outputs, in
# output/ as with runAll.sh, are checked against the same references:
#   scripts/runAllParallel.sh; (wait for output/runCorpus.log and 08.status)
#   scripts/testReviewResults.sh
# Options: --threads N and --cpu-groups "LIST;LIST" (round-robin per scene).
# Each LIST is a taskset CPU list, for example "0-17;18-35".
# Arguments select corpus entries as in runAll.sh (soda runs only with none).
#
# TypeScript: every parallel Jacobi thread is a replica of the program, with
# its own copy of the scene (see RefinementReplicas), and runAll.sh runs the
# whole corpus at once, so the default number of threads is not the number of
# online processors (as in C++) but a small one, whose memory the corpus fits in.
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
exec bash "$SCRIPT_DIR/runAll.sh" "$@"
