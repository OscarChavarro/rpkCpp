#!/bin/bash
findimagedupes -t 100 `ls doc/testBaseImages/*.ppm output/*.ppm | sort` 2> /dev/null | sort > /tmp/x

PATTERN=`pwd -P`/

N=`awk -v pattern="$PATTERN" '{gsub(pattern, ""); print}' /tmp/x | wc -l`

if [ "$N" -eq 12 ]; then
    echo "TEST PASS"
else
    echo "*** TEST ERROR! ***"
    echo "$N found, expected 12"
fi
