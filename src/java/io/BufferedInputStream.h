#ifndef __JAVA_IO_BUFFERED_INPUT_STREAM__
#define __JAVA_IO_BUFFERED_INPUT_STREAM__

#include "java/io/File.h"
#include "java/io/InputStream.h"

namespace java {
namespace io {

class BufferedInputStream : public InputStream {
  private:
    InputStream *inputStream;
    bool ownInputStream;

  public:
    explicit BufferedInputStream(InputStream *inputStream = nullptr, bool ownInputStream = true);
    ~BufferedInputStream() override;

    bool
    isOpen() const;

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
