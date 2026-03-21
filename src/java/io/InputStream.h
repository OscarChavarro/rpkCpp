#ifndef __JAVA_IO_INPUT_STREAM__
#define __JAVA_IO_INPUT_STREAM__

namespace java {
namespace io {

class InputStream {
  public:
    virtual int read() = 0;
    virtual int read(unsigned char *buffer, int offset, int length) = 0;
    virtual bool close() = 0;
    virtual void dispose() { close(); }
    virtual ~InputStream() {}
};

}
}

#endif
