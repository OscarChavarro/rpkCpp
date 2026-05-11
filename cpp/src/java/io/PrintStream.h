#ifndef JAVA_IO_PRINT_STREAM__
#define JAVA_IO_PRINT_STREAM__

#include <cstdarg>

#include "java/io/OutputStream.h"

namespace java {

class PrintStream {
  private:
    OutputStream *stream;

    static void
    writeText(OutputStream *stream, const char *text);

    static void
    writeFormatted(OutputStream *stream, const char *format, va_list arguments);

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

#endif
