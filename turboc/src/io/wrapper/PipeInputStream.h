#ifndef __PIPE_INPUT_STREAM__
#define __PIPE_INPUT_STREAM__

#include <stdio.h>

#include "java/io/InputStream.h"

class PipeInputStream : public InputStream {
  private:
    void *pipeHandle;
    static FILE *toFileHandle(void *handle);

  public:
    explicit PipeInputStream(const char *command);
    ~PipeInputStream();

    bool
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
