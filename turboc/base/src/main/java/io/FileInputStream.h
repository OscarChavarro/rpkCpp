#ifndef __FILE_INPUT_STREAM__
#define __FILE_INPUT_STREAM__

#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/File.h"
#include "java/io/InputStream.h"

class FileInputStream : public InputStream {
  private:
    FILE *stream;

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
