#include <cstdio>
#include <cstring>

#include "java/io/PrintStream.h"
#include "java/util/Formatter.h"

namespace java {

PrintStream::PrintStream(OutputStream *stream):
    stream(stream)
{
}

void
PrintStream::writeText(OutputStream *stream, const char *text) {
    if ( stream == nullptr || text == nullptr ) {
        return;
    }
    const int length = static_cast<int>(std::strlen(text));
    if ( length <= 0 ) {
        return;
    }
    stream->write(reinterpret_cast<const unsigned char *>(text), 0, length);
}

void
PrintStream::writeFormatted(OutputStream *stream, const char *format, va_list arguments) {
    if ( stream == nullptr || format == nullptr ) {
        return;
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);
    if ( required <= 0 ) {
        return;
    }

    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        writeText(stream, localBuffer);
        return;
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);
    writeText(stream, dynamicBuffer);
    delete[] dynamicBuffer;
}

PrintStream &
PrintStream::printf(const char *format, ...) {
    if ( stream == nullptr || format == nullptr ) {
        return *this;
    }
    va_list arguments;
    va_start(arguments, format);
    writeFormatted(stream, format, arguments);
    va_end(arguments);
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
