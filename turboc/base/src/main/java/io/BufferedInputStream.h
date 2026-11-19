#ifndef __BUFFERED_INPUT_STREAM__
#define __BUFFERED_INPUT_STREAM__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/File.h"
#include "java/io/InputStream.h"

class BufferedInputStream : public InputStream {
  protected:
    InputStream *inputStream;

  public:
    explicit BufferedInputStream(InputStream *inputStream = NULL);
    ~BufferedInputStream();

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
