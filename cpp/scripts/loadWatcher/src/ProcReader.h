#ifndef LOAD_WATCHER_PROC_READER_H
#define LOAD_WATCHER_PROC_READER_H

#include <string>

#include "java/util/ArrayList.h"

#include "ProcessSnapshot.h"

class ProcReader {
  private:
    static bool isNumericName(const char *name);
    static bool readProcessName(pid_t pid, std::string *name);
    static bool readCommandLine(pid_t pid, java::ArrayList<std::string> *arguments);
    static std::string chooseInputFile(const java::ArrayList<std::string> &arguments);
    static bool readVirtualBytes(pid_t pid, unsigned long long *virtualBytes);
    static bool readResidentBytes(pid_t pid, unsigned long long *rssBytes);
    static bool readUserSeconds(pid_t pid, double *userSeconds);
    static bool readSnapshot(pid_t pid, ProcessSnapshot *snapshot);

  public:
    static java::ArrayList<ProcessSnapshot> readProcessesByName(const char *processName);
};

#endif
