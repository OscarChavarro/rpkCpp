#ifndef __JAVA_IO_FILE_INPUT_STREAM__
#define __JAVA_IO_FILE_INPUT_STREAM__

#include "java/io/File.h"
#include "java/io/InputStream.h"

namespace java {
namespace io {

class FileInputStream : public InputStream {
  private:
    void *stream;
    bool closeOnDispose;

  public:
    FileInputStream();
    explicit FileInputStream(const char *fileName);
    ~FileInputStream() override;

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
