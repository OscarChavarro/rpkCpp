#ifndef PIPE_INPUT_STREAM__
#define PIPE_INPUT_STREAM__

#include <cstdio>

#include "java/io/InputStream.h"

class PipeInputStream : public java::InputStream {
  private:
    void *pipeHandle;
    static FILE *toFileHandle(void *handle);

  public:
    explicit PipeInputStream(const char *command);
    ~PipeInputStream() override;

    bool
    isOpen() const;

    int
    read() override;

    int
    read(unsigned char *buffer, int offset, int length) override;

    void
    close() override;

    void
    dispose() override;
};

#endif
