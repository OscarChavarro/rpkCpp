#ifndef __PIPE_OUTPUT_STREAM__
#define __PIPE_OUTPUT_STREAM__

#include <stdio.h>

#include "java/io/OutputStream.h"

class PipeOutputStream : public OutputStream {
  private:
    void *pipeHandle;
    static FILE *toFileHandle(void *handle);

  public:
    explicit PipeOutputStream(const char *command);
    ~PipeOutputStream();

    bool
    isOpen() const;

    void
    write(int value);

    void
    write(const unsigned char *buffer, int offset, int length);

    void
    flush();

    void
    close();

    void
    dispose();
};

#endif
