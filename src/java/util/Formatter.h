#ifndef __JAVA_UTIL_FORMATTER__
#define __JAVA_UTIL_FORMATTER__

#include <cstdarg>

#include "java/lang/String.h"

namespace java {
namespace io {
class OutputStream;
}
namespace util {

class Formatter {
  public:
    Formatter();
    explicit Formatter(java::io::OutputStream *outputStream);
    ~Formatter();

    java::io::OutputStream *
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
    static java::lang::String
    appendText(const java::lang::String &left, const java::lang::String &right);

    static java::lang::String
    formatToString(const char *formatText, va_list arguments);

    java::io::OutputStream *outputStream;
    java::lang::String content;
    bool closed;
};

}
}

#endif
