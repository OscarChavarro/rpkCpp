#ifndef __PROCESS_BUILDER__
#define __PROCESS_BUILDER__

namespace java {

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

}

#endif
