#ifndef __PIPE_OUTPUT_STREAM__
#define __PIPE_OUTPUT_STREAM__

#include "java/io/OutputStream.h"

class PipeOutputStream : public java::io::OutputStream {
  private:
    void *pipeHandle;

  public:
    explicit PipeOutputStream(const char *command);
    ~PipeOutputStream() override;

    bool
    isOpen() const;

    void
    write(int value) override;

    void
    write(const unsigned char *buffer, int offset, int length) override;

    void
    flush() override;

    void
    close() override;

    void
    dispose() override;
};

#endif
