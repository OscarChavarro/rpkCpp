#ifndef __JAVA_IO_BUFFERED_INPUT_STREAM__
#define __JAVA_IO_BUFFERED_INPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/io/InputStream.h"

namespace java {
namespace io {

class BufferedInputStream : public InputStream {
  private:
    FileInputStream *inputStream;
    bool ownInputStream;

  public:
    explicit BufferedInputStream(FileInputStream *inputStream = nullptr, bool ownInputStream = true);
    ~BufferedInputStream() override;

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
