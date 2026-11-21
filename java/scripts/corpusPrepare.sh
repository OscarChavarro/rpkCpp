#!/bin/sh
# Sourced by runCorpus.sh and runAll.sh: build once, then let the runs start
# the JVM directly (scripts/runJava.sh) instead of one Gradle run task each, so
# that their CPU time is measured and they do not rebuild concurrently.
PREPARE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
gradle -q :base:buildNativeRandomLibrary :base:jar :testsuite:ApplicationCases:RenderparkApplication:classes || return 1
RPK_JAVA_CLASSPATH=$(gradle -q -I "$PREPARE_DIR/printRuntimeClasspath.gradle" \
    :testsuite:ApplicationCases:RenderparkApplication:printRuntimeClasspath | tail -1) || return 1
export RPK_JAVA_CLASSPATH
export RPK_GRADLE="$PREPARE_DIR/runJava.sh"
