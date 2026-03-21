#ifndef __JAVA_IO_PRINT_STREAM__
#define __JAVA_IO_PRINT_STREAM__

#include <cstdio>

namespace java {
namespace io {

class PrintStream {
  private:
    FILE *stream;

  public:
    explicit PrintStream(FILE *stream);

    void
    setStream(FILE *stream);

    int
    printf(const char *format, ...) const;

    void
    print(const char *text) const;

    void
    println(const char *text) const;

    void
    println() const;

    void
    flush() const;

    static PrintStream &
    out();

    static PrintStream &
    err();
};

}
}

#endif
