#ifndef LOAD_WATCHER_H
#define LOAD_WATCHER_H

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
