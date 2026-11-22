#!/bin/bash
# Shared option parsing and per-process CPU affinity for runAllParallel.sh.
rpk_parallel_options() {
    RPK_AFFINITY_GROUPS="${RPK_AFFINITY_GROUPS:-}"
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --threads|--cpu-groups)
                if [ "$#" -lt 2 ]; then echo "Missing value for $1" >&2; return 2; fi
                case "$1" in
                    --threads) threads="$2" ;;
                    --cpu-groups) RPK_AFFINITY_GROUPS="$2" ;;
                esac
                shift 2 ;;
            --) shift; break ;;
            -*) echo "Unknown option: $1" >&2; return 2 ;;
            *) break ;;
        esac
    done
    case "$threads" in ''|*[!0-9]*|0) echo "Threads must be a positive integer" >&2; return 2 ;; esac
    if [ -n "$RPK_AFFINITY_GROUPS" ]; then
        if ! [[ "$RPK_AFFINITY_GROUPS" =~ ^[0-9,-]+(\;[0-9,-]+)*$ ]]; then
            echo "CPU groups must be semicolon-separated taskset CPU lists" >&2; return 2
        fi
        command -v taskset >/dev/null && command -v flock >/dev/null || { echo "CPU groups require taskset and flock" >&2; return 2; }
        RPK_AFFINITY_COUNTER="$(mktemp "${TMPDIR:-/tmp}/rpk-affinity.XXXXXXXX")" || return 2
        printf '0\n' > "$RPK_AFFINITY_COUNTER"
        export RPK_AFFINITY_GROUPS RPK_AFFINITY_COUNTER
    fi
    RPK_PARALLEL_ARGS=("$@")
}

# Assign successive scene processes round-robin. Children inherit affinity.
rpk_parallel_exec() {
    if [ -z "${RPK_AFFINITY_GROUPS:-}" ]; then exec "$@"; fi
    group=$(
      {
        flock -x 9 || exit 1
        count=$(cat "$RPK_AFFINITY_COUNTER")
        groups=$(printf '%s\n' "$RPK_AFFINITY_GROUPS" | tr ';' '\n' | wc -l)
        group=$(printf '%s\n' "$RPK_AFFINITY_GROUPS" | cut -d ';' -f "$((count % groups + 1))")
        printf '%s\n' "$((count + 1))" > "$RPK_AFFINITY_COUNTER"
        printf '%s\n' "$group"
      } 9>"$RPK_AFFINITY_COUNTER.lock"
    ) || exit 1
    exec taskset -c "$group" "$@"
}
