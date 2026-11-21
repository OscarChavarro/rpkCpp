#!/bin/bash
# Stand-in for gradle in the corpus scripts, set as RPK_GRADLE by
# corpusPrepare.sh. A scene script calls
#   "${RPK_GRADLE}" -q <run task> --args "<program arguments>"
# and this runs the application's JVM directly, as a child of the script, with
# the classpath and JVM options of the RenderparkApplication run task. The JVM
# is then part of the process tree /usr/bin/time measures (corpusRunEntry in
# corpus.sh); through the Gradle daemon its CPU time would not be counted.
set -f
while [ $# -gt 0 ] && [ "$1" != "--args" ]; do
    shift
done
shift
if [ -z "${RPK_JAVA_CLASSPATH:-}" ]; then
    echo "runJava.sh: RPK_JAVA_CLASSPATH is not set (source scripts/corpusPrepare.sh first)" >&2
    exit 2
fi
# Program arguments are split on blanks, as gradle --args does for these scripts
exec java --add-modules jdk.incubator.foreign --enable-native-access=ALL-UNNAMED \
    -cp "${RPK_JAVA_CLASSPATH}" vsdk.toolkit.app.Main $1
