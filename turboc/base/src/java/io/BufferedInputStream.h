#ifndef JAVA_IO_BFFRD_INPT_STRM
#define JAVA_IO_BFFRD_INPT_STRM

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
