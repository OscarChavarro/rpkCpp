#ifndef __PIPE_OUTPUT_STREAM__
#define __PIPE_OUTPUT_STREAM__

#include <cstdio>

#include "java/io/FileOutputStream.h"

class PipeOutputStream : public java::io::FileOutputStream {
  private:
    FILE *pipeHandle;

  public:
    explicit PipeOutputStream(FILE *fileHandle);
    ~PipeOutputStream() override;

    void
    close() override;

    void
    dispose() override;
};

#endif
