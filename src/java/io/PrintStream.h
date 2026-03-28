#ifndef __JAVA_IO_PRINT_STREAM__
#define __JAVA_IO_PRINT_STREAM__

#include "java/io/OutputStream.h"

namespace java {
namespace io {

class PrintStream {
  private:
    OutputStream *stream;

  public:
    explicit PrintStream(OutputStream *stream);

    PrintStream &
    printf(const char *format, ...);

    void
    print(const char *text) const;

    void
    println(const char *text) const;

    void
    println() const;

    void
    flush() const;
};

}
}

#endif
