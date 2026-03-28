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

  private:
    java::io::OutputStream *outputStream;
    java::lang::String content;
    bool closed;
};

int
formatToBuffer(char *buffer, int bufferSize, const char *format, ...);

int
vformatToBuffer(char *buffer, int bufferSize, const char *format, va_list arguments);

java::lang::String
format(const char *format, ...);

java::lang::String
vformat(const char *format, va_list arguments);

}
}

#endif
