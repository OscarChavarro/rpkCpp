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
    explicit FileOutputStream(const char *fileName);
    FileOutputStream(FILE *fileHandle, bool pipeOutput, bool standardOutput = false);
    ~FileOutputStream() override;

    bool
    isOpen() const;

    bool
    isPipeOutput() const;

    bool
    isStandardOutput() const;

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
