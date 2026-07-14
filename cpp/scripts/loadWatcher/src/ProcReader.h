#ifndef LOAD_WATCHER_PROC_READER_H
#define LOAD_WATCHER_PROC_READER_H

#include "java/util/ArrayList.h"

#include "ProcessSnapshot.h"

class ProcReader {
  public:
    static java::ArrayList<ProcessSnapshot> readProcessesByName(const char *processName);
};

#endif
