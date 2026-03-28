#ifndef __PIPE_INPUT_STREAM__
#define __PIPE_INPUT_STREAM__

#include <cstdio>

#include "java/io/FileInputStream.h"

class PipeInputStream : public java::io::FileInputStream {
  private:
    FILE *pipeHandle;

  public:
    explicit PipeInputStream(FILE *fileHandle);
    ~PipeInputStream() override;

    void
    close() override;

    void
    dispose() override;
};

#endif
