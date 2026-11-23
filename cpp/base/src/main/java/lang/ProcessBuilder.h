#ifndef __PROCESS_BUILDER__
#define __PROCESS_BUILDER__

#include <cstdio>

namespace java {

class ProcessBuilder {
  private:
    const char *command;

  public:
    explicit ProcessBuilder(const char *commandLine);

    FILE *startRead() const;
    FILE *startWrite() const;

    static FILE *start(const char *commandLine, const char *mode);
    static int close(FILE *processHandle);
};

}

#endif
