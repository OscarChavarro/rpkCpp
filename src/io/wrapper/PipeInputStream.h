#ifndef __PIPE_INPUT_STREAM__
#define __PIPE_INPUT_STREAM__

#include "java/io/InputStream.h"

class PipeInputStream : public java::io::InputStream {
  private:
    void *pipeHandle;

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
