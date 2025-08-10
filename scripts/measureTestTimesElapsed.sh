#!/bin/bash
ref_file="output/01_cube.ppm"

if [ ! -f "$ref_file" ]; then
    echo "Reference file not found: $ref_file"
    exit 1
fi

ref_epoch=$(stat -c %Y "$ref_file")

for file in output/*.ppm; do
    file_epoch=$(stat -c %Y "$file")
    diff_minutes=$(( (file_epoch - ref_epoch) / 60 ))
    echo "${diff_minutes} minutes: $(basename "$file")"
done
