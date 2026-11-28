#ifndef __BUFFERED_OUTPUT_STREAM__
#define __BUFFERED_OUTPUT_STREAM__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/io/File.h"
#include "java/io/OutputStream.h"

class BufferedOutputStream : public OutputStream {
  protected:
    OutputStream *outputStream;

  public:
    explicit BufferedOutputStream(OutputStream *outputStream = NULL);
    ~BufferedOutputStream();

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
