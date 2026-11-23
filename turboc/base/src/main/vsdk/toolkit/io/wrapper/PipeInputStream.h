#ifndef __PIPE_INPUT_STREAM__
#define __PIPE_INPUT_STREAM__

#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/InputStream.h"
class PipeInputStream : public InputStream {
  private:
    FILE *pipeHandle;

  public:
    explicit PipeInputStream(const char *command);
    ~PipeInputStream();

    BOOLEAN
    isOpen() const;

    int
    read();

    int
    read(unsigned char *buffer, int offset, int length);

    void
    close();

    void
    dispose();
};

#endif
