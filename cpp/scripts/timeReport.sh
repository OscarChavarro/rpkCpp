#!/bin/sh
# Timing report of the last corpus run. Run from cpp/ after scripts/runCorpus.sh
# or scripts/runAll.sh, which write output/<log>.time for every run.
#
#   timeReport.sh [image ...]
#
# Prints, per image, the CPU time of its run (user + system seconds), the wall
# clock and the peak RSS. CPU time is the figure to compare between runs: it
# does not depend on how many runs share the machine, as the wall clock does. A
# scene outside the corpus whose run has not finished is reported as not
# measured.

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/corpus.sh"

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

printf '%-40s %10s %10s %9s\n' "image" "cpu s" "wall s" "rss MB"
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
    printf '%-40s %10s %10s %9s\n' "$image" "$cpu" "$wall" "$rss"
done
