#include "java/io/PrintStream.h"

#include <cstring>

#include "java/util/Formatter.h"

namespace java {
namespace io {

PrintStream::PrintStream(OutputStream *stream):
    stream(stream)
{
}

static void
writeText(OutputStream *stream, const char *text) {
    if ( stream == nullptr || text == nullptr ) {
        return;
    }
    const int length = static_cast<int>(std::strlen(text));
    if ( length <= 0 ) {
        return;
    }
    stream->write(reinterpret_cast<const unsigned char *>(text), 0, length);
}

PrintStream &
PrintStream::printf(const char *format, ...) {
    if ( stream == nullptr || format == nullptr ) {
        return *this;
    }
    va_list arguments;
    va_start(arguments, format);
    const java::lang::String text = java::util::vformat(format, arguments);
    va_end(arguments);
    writeText(stream, text.toCString());
    return *this;
}

void
PrintStream::print(const char *text) const {
    writeText(stream, text);
}

void
PrintStream::println(const char *text) const {
    if ( stream == nullptr ) {
        return;
    }
    writeText(stream, text);
    stream->write('\n');
}

void
PrintStream::println() const {
    if ( stream == nullptr ) {
        return;
    }
    stream->write('\n');
}

void
PrintStream::flush() const {
    if ( stream == nullptr ) {
        return;
    }
    stream->flush();
}

}
}
