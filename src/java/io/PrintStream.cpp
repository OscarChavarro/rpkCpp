#include "java/io/PrintStream.h"

#include <cstdarg>

namespace java {
namespace io {

PrintStream::PrintStream(FILE *stream):
    stream(stream)
{
}

void
PrintStream::setStream(FILE *stream) {
    this->stream = stream;
}

int
PrintStream::printf(const char *format, ...) const {
    if ( stream == nullptr || format == nullptr ) {
        return 0;
    }
    va_list arguments;
    va_start(arguments, format);
    const int count = vfprintf(stream, format, arguments);
    va_end(arguments);
    return count;
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

PrintStream &
PrintStream::out() {
    static PrintStream output(stdout);
    return output;
}

PrintStream &
PrintStream::err() {
    static PrintStream error(stderr);
    return error;
}

}
}
