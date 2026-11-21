#ifndef __FILE_OUTPUT_STREAM__
#define __FILE_OUTPUT_STREAM__

#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/File.h"
#include "java/io/OutputStream.h"

class FileOutputStream : public OutputStream {
  private:
    void *stream;
    BOOLEAN ownsStream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileOutputStream(const char *fileName);
    // An already open stream (i.e. stdout), flushed but not closed by close()
    explicit FileOutputStream(FILE *openStream);
    ~FileOutputStream();

    void
    write(int value);

    void
    write(const unsigned char *buffer, int offset, int length);

    void
    flush();

    void
    close();

    void
    dispose();
};


#endif
