#include "java/io/PrintStream.h"

#include <cstdarg>

namespace java {
namespace io {

PrintStream::PrintStream(FILE *stream):
    stream(stream)
{
}

PrintStream &
PrintStream::printf(const char *format, ...) {
    if ( stream == nullptr || format == nullptr ) {
        return *this;
    }
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stream, format, arguments);
    va_end(arguments);
    return *this;
}

void
PrintStream::print(const char *text) const {
    if ( stream == nullptr || text == nullptr ) {
        return;
    }
    fprintf(stream, "%s", text);
}

void
PrintStream::println(const char *text) const {
    if ( stream == nullptr ) {
        return;
    }
    if ( text == nullptr ) {
        fprintf(stream, "\n");
    } else {
        fprintf(stream, "%s\n", text);
    }
}

void
PrintStream::println() const {
    if ( stream == nullptr ) {
        return;
    }
    fprintf(stream, "\n");
}

void
PrintStream::flush() const {
    if ( stream == nullptr ) {
        return;
    }
    fflush(stream);
}

}
}
