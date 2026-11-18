#ifndef __FORMATTER__
#define __FORMATTER__

#include <cstdarg>

#include "java/io/OutputStream.h"
#include "java/lang/String.h"
namespace java {
class Formatter {
  public:
    Formatter();
    explicit Formatter(java::OutputStream *outputStream);
    ~Formatter();

    java::OutputStream *
    out() const;

    void
    flush();

    void
    close();

    Formatter &
    format(const char *format, ...);

    static int
    format(char *buffer, int bufferSize, const char *format, ...);

    static int
    vformat(char *buffer, int bufferSize, const char *format, va_list arguments);

  private:
    static java::String
    appendText(const java::String &left, const java::String &right);

    static java::String
    formatToString(const char *formatText, va_list arguments);

    java::OutputStream *outputStream;
    java::String content;
    bool closed;
};

}

#endif
