#ifndef LOAD_WATCHER_PROC_READER_H
#define LOAD_WATCHER_PROC_READER_H

#include <vector>

#include "ProcessSnapshot.h"

class ProcReader {
  public:
    static std::vector<ProcessSnapshot> readProcessesByName(const char *processName);
};

#endif
