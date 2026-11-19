#ifndef __PIPE_OUTPUT_STREAM__
#define __PIPE_OUTPUT_STREAM__

#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/OutputStream.h"
class PipeOutputStream : public OutputStream {
  private:
    void *pipeHandle;
    static FILE *toFileHandle(void *handle);

  public:
    explicit PipeOutputStream(const char *command);
    ~PipeOutputStream();

    BOOLEAN
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
