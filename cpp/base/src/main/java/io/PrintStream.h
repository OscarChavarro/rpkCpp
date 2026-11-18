#ifndef __PRINT_STREAM__
#define __PRINT_STREAM__

#include <cstdarg>

#include "java/io/OutputStream.h"
namespace java {

class PrintStream {
  private:
    OutputStream *stream;
    bool autoFlush;

    static void
    writeText(OutputStream *stream, const char *text);

    static void
    writeFormatted(OutputStream *stream, const char *format, va_list arguments);

  public:
    explicit PrintStream(OutputStream *stream);

    /**
    @param autoFlush as Java: when true, the stream is flushed after each
    `println` and each `printf`
    */
    PrintStream(OutputStream *stream, bool autoFlush);

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
