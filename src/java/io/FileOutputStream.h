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
    int isPipe;
    bool standardOutput;

  public:
    FileOutputStream();
    explicit FileOutputStream(const File &file);
    ~FileOutputStream() override;

    bool
    open(const File &file);

    bool
    open(const char *fileName);

    bool
    open(FILE *fileHandle, bool pipeOutput);

    bool
    openCompressed(const File &file);

    bool
    openCompressed(const char *fileName);

    bool
    openStandardOutput();

    bool
    isOpen() const;

    bool
    isPipeOutput() const;

    bool
    isStandardOutput() const;

    int
    write(int value) override;

    int
    write(const unsigned char *buffer, int offset, int length) override;

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
