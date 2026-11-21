#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "app/RpkApplication.h"
#include "RayCorpusReplay.h"

/**
C++ port note: the C++ port parses the numbers with atoll into long long;
this port parses them with strtod into doubles holding whole numbers
(exact up to 2^53), since Turbo C++ 3.0 has no 64-bit integer
*/
static double
environmentNumber(const char *name, const double defaultValue) {
    const char *value = getenv(name);
    return value != NULL && value[0] != '\0' ? floor(strtod(value, NULL)) : defaultValue;
}

/**
Replays a step 1 ray corpus against the scene built from the same command
line as the run that captured it (doc/vitralAlignmentPlan.md, step 4 item 4).
The command line is the rpk one; the replay is configured by environment, so
that scripts/replayRays.sh can run each corpus scene's own script with this
program as RPK_BIN:

  RPK_RAY_REPLAY_FILE           corpus file (required)
  RPK_RAY_REPLAY_REPORT         file receiving each disagreement with its ray
  RPK_RAY_REPLAY_THREADS        workers of the worker pass (default 4)
  RPK_RAY_REPLAY_LINEAR_STRIDE  replay 1 query in N through the linear
                                reference (default 1, all)
  RPK_RAY_REPLAY_LIMIT          stop after N replayed queries (default all)
*/
int
main(int argc, char *argv[]) {
    const char *corpus = getenv("RPK_RAY_REPLAY_FILE");
    if ( corpus == NULL || corpus[0] == '\0' ) {
        fprintf(stderr, "rayCorpusReplay: set RPK_RAY_REPLAY_FILE to a ray corpus (.vqc)\n");
        return 2;
    }
    RayCorpusReplay replay(
        corpus,
        getenv("RPK_RAY_REPLAY_REPORT"),
        ((int)(environmentNumber("RPK_RAY_REPLAY_THREADS", 4.0))),
        environmentNumber("RPK_RAY_REPLAY_LINEAR_STRIDE", 1.0),
        environmentNumber("RPK_RAY_REPLAY_LIMIT", 0.0));
    RpkApplication *application = new RpkApplication();
    const int status = application->entryPoint(argc, argv, &replay);
    delete application;
    return status;
}
