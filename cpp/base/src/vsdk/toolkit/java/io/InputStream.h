#ifndef JAVA_IO_INPUT_STREAM__
#define JAVA_IO_INPUT_STREAM__

namespace java {

class InputStream {
  public:
    virtual int read() = 0;
    virtual int read(unsigned char *buffer, int offset, int length) = 0;
    virtual void close() = 0;
    virtual void dispose();
    virtual ~InputStream();
};

}

#endif
