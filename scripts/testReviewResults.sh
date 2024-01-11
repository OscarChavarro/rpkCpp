#!/bin/bash
findimagedupes -t 100 `ls doc/testBaseImages/*.ppm output/*.ppm | sort` 2> /dev/null | sort > /tmp/x

PATTERN=`pwd -P`/

N=`awk -v pattern="$PATTERN" '{gsub(pattern, ""); print}' /tmp/x | wc -l`

if [ "$N" -eq 12 ]; then
    echo "TEST PASS"
else
    echo "*** TEST ERROR! ***"
    echo "$N found, expected 12:"
    rm -f /tmp/y
    while IFS= read -r line; do
	underscore_position=$(awk -F'_' '{print length($0) - length($NF)}' <<< "$line")

        if [ "$underscore_position" -gt 2 ]; then
            extracted_text="${line:underscore_position-3:2}"
            echo "$extracted_text" >> /tmp/y
        fi
    done < /tmp/x
    sort -n /tmp/y
fi
