#ifndef __JAVA_IO_FILE_OUTPUT_STREAM__
#define __JAVA_IO_FILE_OUTPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/OutputStream.h"

namespace java {
namespace io {

class FileOutputStream : public OutputStream {
  private:
    FILE *stream;
    bool closeOnDispose;

  public:
    FileOutputStream();
    explicit FileOutputStream(const File &file);
    explicit FileOutputStream(const char *fileName);
    explicit FileOutputStream(FILE *fileHandle, bool closeOnDispose = true);
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
    close() override;

    void
    dispose() override;
};

}
}

#endif
