#ifndef __BUFFERED_INPUT_STREAM__
#define __BUFFERED_INPUT_STREAM__

#include "java/io/File.h"
#include "java/io/InputStream.h"
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
