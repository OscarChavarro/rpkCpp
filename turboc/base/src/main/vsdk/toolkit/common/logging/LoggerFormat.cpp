#include <stdarg.h>

#include "vsdk/toolkit/common/logging/LoggerFormat.h"

String
LoggerFormat::format(const char *text, ...) {
    va_list arguments;

    va_start(arguments, text);
    const String message = String::formatCStringToJavaString(text, arguments);
    va_end(arguments);
    return message;
}
