#ifndef JAVA_IO_BUFFERED_INPUT_STREAM__
#define JAVA_IO_BUFFERED_INPUT_STREAM__

#include "vsdk/toolkit/java/io/File.h"
#include "vsdk/toolkit/java/io/InputStream.h"

namespace java {

class BufferedInputStream : public InputStream {
  protected:
    InputStream *inputStream;

  public:
    explicit BufferedInputStream(InputStream *inputStream = nullptr);
    ~BufferedInputStream() override;

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
