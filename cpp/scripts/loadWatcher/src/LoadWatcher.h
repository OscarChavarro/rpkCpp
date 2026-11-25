#ifndef __LOAD_WATCHER__
#define __LOAD_WATCHER__

#include "ProcessSnapshot.h"

class LoadWatcher {
  private:
    static const char *const TARGET_PROCESS_NAME;
    static constexpr unsigned int POLL_INTERVAL_SECONDS = 1;

    static void printProcessReport(const ProcessStats &stats);

  public:
    int run() const;
};

#endif
