#!/bin/sh
# Timing report of the last corpus run. Run from cpp/ after scripts/runCorpus.sh
# or scripts/runAll.sh, which write output/<log>.time for every run.
#
#   timeReport.sh [--record] [image ...]
#
# Prints, per image, the CPU time of its run (user + system seconds), the wall
# clock and the peak RSS, and the change of CPU time against the baseline in
# etc/goldenImagesForTests/timings.txt. CPU time is the figure to compare: it does not
# depend on how many runs share the machine, as the wall clock does. A scene
# outside the corpus whose run has not finished is reported as not measured.
# --record stores the current CPU times as the baseline (all selected images
# with a finished run).

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

baseline="etc/goldenImagesForTests/timings.txt"

record=0
if [ "${1:-}" = "--record" ]; then
    record=1
    shift
fi

selection=$(corpusSelect "$@")
if [ $# -eq 0 ]; then
    selection=$(printf '%s\n%s\n' "$selection" "$OPTIONAL_TABLE")
fi
if [ -z "$selection" ]; then
    echo "No corpus entry matches: $*"
    exit 2
fi

# Value of one field (user, system, elapsed, maxRssKb) of a time file
timeField() {
    awk -v k="$2" '$1 == k { print $2 }' "$1"
}

if [ "$record" -eq 1 ]; then
    touch "$baseline"
fi

printf '%-40s %10s %10s %9s %10s %8s\n' "image" "cpu s" "wall s" "rss MB" "base cpu" "change"
for line in $selection; do
    image=$(echo "$line" | cut -d'|' -f1)
    log=$(echo "$line" | cut -d'|' -f3)
    file="output/$log.time"
    if [ ! -f "output/$log.status" ] || [ ! -f "$file" ]; then
        printf '%-40s %s\n' "$image" "not measured (run not finished, or started without GNU time)"
        continue
    fi
    if [ "$(cat "output/$log.status")" != "0" ]; then
        printf '%-40s %s\n' "$image" "not measured (run ended with exit status $(cat "output/$log.status"))"
        continue
    fi
    cpu=$(awk -v u="$(timeField "$file" user)" -v s="$(timeField "$file" system)" 'BEGIN { printf "%.2f", u + s }')
    wall=$(timeField "$file" elapsed)
    rss=$(awk -v k="$(timeField "$file" maxRssKb)" 'BEGIN { printf "%.0f", k / 1024 }')
    base=$(awk -v i="$image" '$1 == i { print $2 }' "$baseline" 2>/dev/null)
    if [ -n "$base" ]; then
        change=$(awk -v c="$cpu" -v b="$base" 'BEGIN { if ( b > 0 ) printf "%+.1f%%", 100 * (c - b) / b; else print "-" }')
    else
        base="-"
        change="-"
    fi
    printf '%-40s %10s %10s %9s %10s %8s\n' "$image" "$cpu" "$wall" "$rss" "$base" "$change"
    if [ "$record" -eq 1 ]; then
        grep -v "^$image " "$baseline" > "$baseline.tmp"
        echo "$image $cpu" >> "$baseline.tmp"
        sort "$baseline.tmp" > "$baseline"
        rm -f "$baseline.tmp"
    fi
done
if [ "$record" -eq 1 ]; then
    echo "Baseline recorded in $baseline"
fi
