#ifndef JAVA_IO_FILE_OUTPUT_STREAM__
#define JAVA_IO_FILE_OUTPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/OutputStream.h"

namespace java {

class FileOutputStream : public OutputStream {
  private:
    void *stream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileOutputStream(const char *fileName);
    ~FileOutputStream() override;

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

}

#endif
