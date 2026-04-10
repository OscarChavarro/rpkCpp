#ifndef __JAVA_IO_FILE_INPUT_STREAM__
#define __JAVA_IO_FILE_INPUT_STREAM__

#include <stdio.h>

#include "java/io/File.h"
#include "java/io/InputStream.h"


class FileInputStream : public InputStream {
  private:
    void *stream;

    static FILE *
    toFileHandle(void *handle);

  public:
    explicit FileInputStream(const char *fileName);
    ~FileInputStream();

    int
    read();

    int
    read(unsigned char *buffer, int offset, int length);

    void
    close();

    void
    dispose();
};


#endif
