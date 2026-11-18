#ifndef __PIPE_OUTPUT_STREAM__
#define __PIPE_OUTPUT_STREAM__

#include <cstdio>

#include "java/io/OutputStream.h"
class PipeOutputStream : public java::OutputStream {
  private:
    void *pipeHandle;
    static FILE *toFileHandle(void *handle);

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
