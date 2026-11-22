#!/bin/bash
# runAll.sh with parallel Jacobi: the Galerkin runs that iterate with Jacobi
# (given with -gr-iteration-method jacobi, or by default) run with
# -gr-iteration-method parallel-jacobi -gr-threads N, N being $RPK_THREADS or,
# by default, the number of online processors (nproc). Gauss-Seidel and
# Southwell runs, and the runs of other methods, are unchanged.
# Parallel Jacobi gives the serial result bit for bit, so the outputs, in
# output/ as with runAll.sh, are checked against the same references:
#   scripts/runAllParallel.sh; (wait for output/runCorpus.log)
#   scripts/testReviewResults.sh
# Arguments select corpus entries as in runAll.sh.
#
# Java: the scene scripts start the program as "${RPK_GRADLE}" -q <task>
# --args "<program arguments>", RPK_GRADLE being scripts/runJava.sh once
# corpusPrepare.sh ran; the wrapper below takes its place (RPK_RUN_WRAPPER)
# and passes runJava.sh the program arguments with the parallel Jacobi options
# appended (the last option wins).

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.." || exit 1
mkdir -p output

threads="${RPK_THREADS:-$(nproc)}"
program="$SCRIPT_DIR/runJava.sh"

wrapper="$(pwd)/output/parallelJacobiRpk.sh"
cat > "$wrapper" <<WRAPPER
#!/bin/sh
set -f
while [ \$# -gt 0 ] && [ "\$1" != "--args" ]; do
    shift
done
shift
arguments="\$1"
galerkin=0
jacobi=1
previous=""
for argument in \$arguments; do
    if [ "\$previous" = "-radiance-method" ] && [ "\$argument" = "Galerkin" ]; then
        galerkin=1
    fi
    if [ "\$previous" = "-gr-iteration-method" ]; then
        case "\$argument" in
            ja*|JA*|Ja*) jacobi=1 ;;
            *) jacobi=0 ;;
        esac
    fi
    previous="\$argument"
done
if [ \$galerkin -eq 1 ] && [ \$jacobi -eq 1 ]; then
    exec "$program" --args "\$arguments -gr-iteration-method parallel-jacobi -gr-threads $threads"
fi
exec "$program" --args "\$arguments"
WRAPPER
chmod +x "$wrapper"

export RPK_RUN_WRAPPER="$wrapper"
exec bash "$SCRIPT_DIR/runAll.sh" "$@"
