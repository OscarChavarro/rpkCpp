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
    ~FileInputStream() override;

    bool
    open(const File &file);

    bool
    open(const char *fileName);

    bool
    openStandardInput();

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

    int
    readLine(char *buffer, int maxLength);

    long
    tell() const;

    bool
    seek(long offset);

    FILE *
    nativeHandle() const;

    bool
    close() override;

    void
    dispose() override;
};

}
}

#endif
