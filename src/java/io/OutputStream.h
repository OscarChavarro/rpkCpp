#ifndef __JAVA_IO_OUTPUT_STREAM__
#define __JAVA_IO_OUTPUT_STREAM__

namespace java {
namespace io {

class OutputStream {
  public:
    virtual int write(int value) = 0;
    virtual int write(const unsigned char *buffer, int offset, int length) = 0;
    virtual bool close() = 0;
    virtual void dispose() { close(); }
    virtual ~OutputStream() {}
};

}
}

#endif
