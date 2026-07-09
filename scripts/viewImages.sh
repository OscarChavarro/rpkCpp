#!/usr/bin/env bash
set -euo pipefail

# Compares the .ppm images produced by each port's runAll.sh (cpp, java,
# turboc, typescript) against the shared reference images in
# doc/testBaseImages, side by side, with a per-port difference heat-map.
# Adapted from povray_1.0_1992/povCpp/scripts/viewImages.sh: same heat-map
# and AE-metric approach, generalized to lay out N ports per scene instead
# of a single output dir, and to tolerate ports that have not rendered a
# given scene yet (skip that port's column instead of failing the run).

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

REFERENCE_DIR="${REFERENCE_DIR:-${ROOT_DIR}/doc/testBaseImages}"
TARGET_DIR="${TARGET_DIR:-/tmp/i}"
PORTS="${PORTS:-cpp java turboc typescript}"

# Difference visualization controls (see povCpp/scripts/viewImages.sh).
DIFF_GAMMA="${DIFF_GAMMA:-2.2}"
DIFF_GRAYSCALE="${DIFF_GRAYSCALE:-RMS}"

# With -skip, scenes where every port present is below the AE threshold are
# omitted from the final listing. `-skip` alone is shorthand for `-skip 1`,
# i.e. skip only scenes where all present ports are a pixel-perfect match.
SKIP_THRESHOLD=0
if [[ "${1:-}" == "-skip" ]]; then
  SKIP_THRESHOLD=1
  if [[ -n "${2:-}" ]]; then
    if [[ ! "${2}" =~ ^[0-9]+$ ]]; then
      echo "Invalid -skip threshold: ${2} (expected non-negative integer)" >&2
      exit 1
    fi
    SKIP_THRESHOLD="${2}"
  fi
fi

if [[ ! -d "${REFERENCE_DIR}" ]]; then
  echo "Missing reference directory: ${REFERENCE_DIR}" >&2
  exit 1
fi

if command -v magick >/dev/null 2>&1; then
  CONVERTER_BIN="magick"
  COMPARE_BIN="magick compare"
elif command -v convert >/dev/null 2>&1; then
  CONVERTER_BIN="convert"
  COMPARE_BIN="compare"
else
  echo "Neither 'magick' nor 'convert' is available in PATH" >&2
  exit 1
fi

# scene label : output file (relative to <port>/output) : reference file
# (relative to REFERENCE_DIR). Mirrors the check_image table in each port's
# scripts/testReviewResults.sh, since output filenames and reference
# filenames are not identical (e.g. both cube variants share one reference).
SCENES=(
  "01_cubeJacobi:01_cubeJacobi.ppm:01_cube.ppm"
  "01_cubeGaussSiedel:01_cubeGaussSiedel.ppm:01_cube.ppm"
  "01_cubeSouthwell:01_cubeSouthwell.ppm:01_cubeSouthwell.ppm"
  "02_corridor:02_corridor.ppm:02_corridor.ppm"
  "03_hospital:03_hospital.ppm:03_hospital.ppm"
  "04_office1:04_office1.ppm:04_office1.ppm"
  "05_office2:05_office2.ppm:05_office2.ppm"
  "06_office3:06_office3.ppm:06_office3.ppm"
  "07_classroom:07_classroom.ppm:07_classroom.ppm"
  "08_soda:08_soda.ppm:08_soda.ppm"
  "10_floorRayMatting:10_floorRayMatting.ppm:10_floorRayMatting.ppm"
  "11_floorRayCasting:11_floorRayCasting.ppm:11_floorRayCasting.ppm"
  "12_floorBidirectionalPathTracing:12_floorBidirectionalPathTracing.ppm:12_floorBidirectionalPathTracing.ppm"
  "13_floorStochasticRaytracingRandomWalk:13_floorStochasticRaytracingRandomWalk.ppm:13_floorStochasticRaytracingRandomWalk.ppm"
  "14_floorStochasticRaytracingJacobi:14_floorStochasticRaytracingJacobi.ppm:14_floorStochasticRaytracingJacobi.ppm"
  "21_tonemapWard:21_tonemapWard.ppm:21_tonemapWard.ppm"
  "22_tonemapTumblinRushmeier:22_tonemapTumblinRushmeier.ppm:22_tonemapTumblinRushmeier.ppm"
)

rm -rf "${TARGET_DIR}"
mkdir -p "${TARGET_DIR}"

work_dir="$(mktemp -d /tmp/viewImages.XXXXXX)"
trap 'rm -rf "${work_dir}"' EXIT

# Heat-map colour lookup table: black (no difference) -> blue -> cyan ->
# yellow -> red (maximum difference). See povCpp/scripts/viewImages.sh.
clut_file="${work_dir}/heat.clut.png"
"${CONVERTER_BIN}" \
  \( -size 1x64 gradient:black-blue \) \
  \( -size 1x64 gradient:blue-cyan \) \
  \( -size 1x64 gradient:cyan-yellow \) \
  \( -size 1x64 gradient:yellow-red \) \
  -append -rotate -90 "${clut_file}"

process_scene() {
  local scene_entry="$1"
  local scene_label output_name reference_name reference_file
  IFS=':' read -r scene_label output_name reference_name <<< "${scene_entry}"

  reference_file="${REFERENCE_DIR}/${reference_name}"
  if [[ ! -f "${reference_file}" ]]; then
    echo "Missing reference image for ${scene_label}: ${reference_file}" >&2
    return 0
  fi

  local reference_size
  reference_size="$("${CONVERTER_BIN}" "${reference_file}" -format '%wx%h' info:)"

  local port_sections=()
  local port_labels=()
  local port

  for port in ${PORTS}; do
    local output_file="${ROOT_DIR}/${port}/output/${output_name}"
    if [[ ! -f "${output_file}" ]]; then
      port_labels+=("${port}=MISSING")
      continue
    fi

    local output_size
    output_size="$("${CONVERTER_BIN}" "${output_file}" -format '%wx%h' info:)"
    if [[ "${reference_size}" != "${output_size}" ]]; then
      port_labels+=("${port}=SIZE ${output_size}!=${reference_size}")
      continue
    fi

    local diff_file="${work_dir}/${scene_label}.${port}.diff.png"

    # Per-pixel colour distance -> single magnitude -> gamma lift -> heat-map.
    "${CONVERTER_BIN}" "${reference_file}" "${output_file}" \
      -alpha off -compose difference -composite \
      -grayscale "${DIFF_GRAYSCALE}" \
      -gamma "${DIFF_GAMMA}" \
      "${clut_file}" -clut \
      "${diff_file}"

    local metric
    # AE is a pixel count but ImageMagick prints it in scientific notation
    # once it gets large (e.g. "2.00648e+06"), so keep it as a raw number
    # string rather than requiring a plain integer match.
    metric="$(${COMPARE_BIN} -metric AE "${reference_file}" "${output_file}" null: 2>&1 || true)"
    metric="${metric%% *}"
    if ! awk -v m="${metric}" 'BEGIN{exit !(m == m + 0)}' 2>/dev/null; then
      metric=""
    fi
    printf '%s' "${metric}" > "${work_dir}/${scene_label}.${port}.metric"

    local port_panel="${work_dir}/${scene_label}.${port}.panel.png"
    "${CONVERTER_BIN}" "${output_file}" "${diff_file}" +append \
      -gravity NorthWest -pointsize 64 -fill red -stroke black -strokewidth 2 \
      -annotate +10+10 "${port}" \
      "${port_panel}"
    port_sections+=("${port_panel}")
    if [[ "${metric}" == "0" ]]; then
      port_labels+=("${port}=match")
    else
      port_labels+=("${port}=AE=${metric:-?}")
    fi
  done

  # Lay out port sections two per row (2 up, 2 down for the default 4
  # ports) instead of one long horizontal strip, so wide port sets stay
  # a reasonable image size. Reference image gets its own row on top.
  local rows=()
  local i
  for ((i = 0; i < ${#port_sections[@]}; i += 2)); do
    if (( i + 1 < ${#port_sections[@]} )); then
      local row_file="${work_dir}/${scene_label}.row$((i / 2)).png"
      "${CONVERTER_BIN}" "${port_sections[i]}" "${port_sections[i + 1]}" +append "${row_file}"
      rows+=("${row_file}")
    else
      rows+=("${port_sections[i]}")
    fi
  done

  local comparison_file="${TARGET_DIR}/${scene_label}.png"
  "${CONVERTER_BIN}" "${reference_file}" "${rows[@]}" -append "${comparison_file}"

  local labels_joined
  labels_joined="$(IFS=' '; echo "${port_labels[*]}")"
  printf '%s\n' "${labels_joined}" > "${work_dir}/${scene_label}.labels"
}

export ROOT_DIR REFERENCE_DIR TARGET_DIR work_dir CONVERTER_BIN COMPARE_BIN
export clut_file DIFF_GAMMA DIFF_GRAYSCALE PORTS
export -f process_scene

max_jobs="$(nproc)"
active_jobs=0
job_failed=0
for scene_entry in "${SCENES[@]}"; do
  bash -c 'set -euo pipefail; process_scene "$1"' _ "${scene_entry}" &
  active_jobs=$((active_jobs + 1))

  if [[ "${active_jobs}" -ge "${max_jobs}" ]]; then
    if ! wait -n; then
      job_failed=1
    fi
    active_jobs=$((active_jobs - 1))
  fi
done

while [[ "${active_jobs}" -gt 0 ]]; do
  if ! wait; then
    job_failed=1
  fi
  active_jobs=0
done

for scene_entry in "${SCENES[@]}"; do
  scene_label="${scene_entry%%:*}"
  comparison_file="${TARGET_DIR}/${scene_label}.png"
  [[ -f "${comparison_file}" ]] || continue

  labels="$(cat "${work_dir}/${scene_label}.labels" 2>/dev/null || true)"

  # Skip the scene only if every present port is at or above SKIP_THRESHOLD
  # of a mismatch, i.e. nothing in the row differs enough to be worth
  # showing (mirrors povCpp/scripts/viewImages.sh's -skip semantics).
  if [[ "${SKIP_THRESHOLD}" -gt 0 ]]; then
    all_below_threshold=1
    for port in ${PORTS}; do
      metric_file="${work_dir}/${scene_label}.${port}.metric"
      [[ -f "${metric_file}" ]] || continue
      metric="$(cat "${metric_file}")"
      if [[ -z "${metric}" ]] || ! awk -v m="${metric}" -v t="${SKIP_THRESHOLD}" 'BEGIN{exit !(m < t)}'; then
        all_below_threshold=0
        break
      fi
    done
    if [[ "${all_below_threshold}" -eq 1 ]]; then
      continue
    fi
  fi

  echo ""
  echo "${comparison_file} ${labels}"
  img2sixel "${comparison_file}"
done

if [[ "${job_failed}" -ne 0 ]]; then
  exit 1
fi

echo ""
