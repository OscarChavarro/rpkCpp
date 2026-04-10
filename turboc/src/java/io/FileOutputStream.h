#ifndef __JAVA_IO_FILE_OUTPUT_STREAM__
#define __JAVA_IO_FILE_OUTPUT_STREAM__

#include <stdio.h>

#include "java/io/File.h"
#include "java/io/OutputStream.h"


class FileOutputStream : public OutputStream {
  private:
    void *stream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileOutputStream(const char *fileName);
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
