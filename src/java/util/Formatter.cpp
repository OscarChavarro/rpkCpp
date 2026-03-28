#include "java/util/Formatter.h"

#include <cstdio>

namespace java {
namespace util {

int
Formatter::vformatToBuffer(char *buffer, int bufferSize, const char *format, va_list arguments) {
    if ( buffer == nullptr || bufferSize <= 0 || format == nullptr ) {
        return -1;
    }
    const int written = std::vsnprintf(buffer, static_cast<std::size_t>(bufferSize), format, arguments);
    if ( written < 0 ) {
        buffer[0] = '\0';
    }
    return written;
}

int
Formatter::formatToBuffer(char *buffer, int bufferSize, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const int written = vformatToBuffer(buffer, bufferSize, format, arguments);
    va_end(arguments);
    return written;
}

java::lang::String
Formatter::vformat(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::lang::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = std::vsnprintf(localBuffer, sizeof(localBuffer), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required <= 0 ) {
        return java::lang::String();
    }

    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::lang::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    std::vsnprintf(dynamicBuffer, static_cast<std::size_t>(required + 1), format, argumentsCopy);
    va_end(argumentsCopy);

    java::lang::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

java::lang::String
Formatter::format(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    java::lang::String result = vformat(format, arguments);
    va_end(arguments);
    return result;
}

}
}
