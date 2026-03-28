#ifndef __JAVA_UTIL_FORMATTER__
#define __JAVA_UTIL_FORMATTER__

#include <cstdarg>

#include "java/lang/String.h"

namespace java {
namespace util {

class Formatter {
  public:
    static int
    formatToBuffer(char *buffer, int bufferSize, const char *format, ...);

    static int
    vformatToBuffer(char *buffer, int bufferSize, const char *format, va_list arguments);

    static java::lang::String
    format(const char *format, ...);

    static java::lang::String
    vformat(const char *format, va_list arguments);
};

}
}

#endif
