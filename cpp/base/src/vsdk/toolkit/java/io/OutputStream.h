#ifndef JAVA_IO_OUTPUT_STREAM__
#define JAVA_IO_OUTPUT_STREAM__

namespace java {

class OutputStream {
  public:
    virtual void write(int value) = 0;
    virtual void write(const unsigned char *buffer, int offset, int length) = 0;
    virtual void flush();
    virtual void close() = 0;
    virtual void dispose();
    virtual ~OutputStream();
};

}

#endif
