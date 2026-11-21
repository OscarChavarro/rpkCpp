#include <cstdlib>
#include <cstdio>
#include "RpkApplication.h"
#include "RayCorpusReplay.h"

static long long
environmentNumber(const char *name, const long long defaultValue) {
    const char *value = getenv(name);
    return value != nullptr && value[0] != '\0' ? atoll(value) : defaultValue;
}

/**
Replays a step 1 ray corpus against the scene built from the same command
line as the run that captured it (doc/vitralAlignmentPlan.md, step 4 item 4).
The command line is the rpk one; the replay is configured by environment, so
that scripts/replayRays.sh can run each corpus scene's own script with this
program as RPK_BIN:

  RPK_RAY_REPLAY_FILE           corpus file (required)
  RPK_RAY_REPLAY_REPORT         file receiving each disagreement with its ray
  RPK_RAY_REPLAY_THREADS        workers of the threaded pass (default 4)
  RPK_RAY_REPLAY_LINEAR_STRIDE  replay 1 query in N through the linear
                                reference (default 1, all)
  RPK_RAY_REPLAY_LIMIT          stop after N replayed queries (default all)
*/
int
main(int argc, char *argv[]) {
    const char *corpus = getenv("RPK_RAY_REPLAY_FILE");
    if ( corpus == nullptr || corpus[0] == '\0' ) {
        fprintf(stderr, "rayCorpusReplay: set RPK_RAY_REPLAY_FILE to a ray corpus (.vqc)\n");
        return 2;
    }
    RayCorpusReplay replay(
        corpus,
        getenv("RPK_RAY_REPLAY_REPORT"),
        static_cast<int>(environmentNumber("RPK_RAY_REPLAY_THREADS", 4)),
        environmentNumber("RPK_RAY_REPLAY_LINEAR_STRIDE", 1),
        environmentNumber("RPK_RAY_REPLAY_LIMIT", 0));
    RpkApplication *application = new RpkApplication();
    const int status = application->entryPoint(argc, argv, &replay);
    delete application;
    return status;
}
