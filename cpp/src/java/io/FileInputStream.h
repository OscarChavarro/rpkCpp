#ifndef JAVA_IO_FILE_INPUT_STREAM__
#define JAVA_IO_FILE_INPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/InputStream.h"

namespace java {

class FileInputStream : public InputStream {
  private:
    void *stream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileInputStream(const char *fileName);
    ~FileInputStream() override;

    int
    read() override;

    int
    read(unsigned char *buffer, int offset, int length) override;

    void
    close() override;

    void
    dispose() override;
};

}

#endif
