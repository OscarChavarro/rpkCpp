#ifndef __FILE_OUTPUT_STREAM__
#define __FILE_OUTPUT_STREAM__

#include <cstdio>

#include "java/io/File.h"
#include "java/io/OutputStream.h"
namespace java {

class FileOutputStream : public OutputStream {
  private:
    void *stream;
    /// False for a wrapped stream (see `FileOutputStream(FILE*)`)
    bool ownsStream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileOutputStream(const char *fileName);

    /**
    Writes to a stream that is already open (as Java's `FileOutputStream`
    over the `FileDescriptor.out`/`err` of `System.out`/`err`), sharing its
    buffer and position with the C stdio functions that use it. The stream
    is not closed by this object.
    @param openStream i.e. `stdout` or `stderr`
    */
    explicit FileOutputStream(FILE *openStream);
    ~FileOutputStream() override;

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

#endif
