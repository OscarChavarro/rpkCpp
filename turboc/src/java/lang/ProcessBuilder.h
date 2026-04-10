#ifndef __JAVA_LANG_PROCESS_BUILDER__
#define __JAVA_LANG_PROCESS_BUILDER__

#include "common/VSDK.h"


class ProcessBuilder {
  private:
    const char *command;

  public:
    explicit ProcessBuilder(const char *commandLine);

    void *startRead() const;
    void *startWrite() const;

    static void *start(const char *commandLine, const char *mode);
    static int close(void *processHandle);
};


#endif
