#ifndef __JAVA_IO_FILE_OUTPUT_STREAM__
#define __JAVA_IO_FILE_OUTPUT_STREAM__

#include "java/io/File.h"
#include "java/io/OutputStream.h"

namespace java {
namespace io {

class FileOutputStream : public OutputStream {
  private:
    void *stream;
    bool closeOnDispose;

  public:
    FileOutputStream();
    explicit FileOutputStream(const char *fileName);
    ~FileOutputStream() override;

    bool
    isOpen() const;

    bool
    ownsHandle() const;

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
}

#endif
