#ifndef __JAVA_IO_FILE_INPUT_STREAM__
#define __JAVA_IO_FILE_INPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/InputStream.h"

namespace java {
namespace io {

class FileInputStream : public InputStream {
  private:
    FILE *stream;
    int isPipe;
    bool standardInput;

  public:
    FileInputStream();
    explicit FileInputStream(const File &file);
    explicit FileInputStream(const char *fileName);
    FileInputStream(FILE *fileHandle, bool pipeInput, bool standardInput = false);
    ~FileInputStream() override;

    bool
    isOpen() const;

    bool
    isPipeInput() const;

    bool
    isStandardInput() const;

    int
    read() override;

    int
    read(unsigned char *buffer, int offset, int length) override;

    long
    tell() const;

    void
    close() override;

    void
    dispose() override;
};

}
}

#endif
