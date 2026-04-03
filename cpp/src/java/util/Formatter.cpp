#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "java/io/OutputStream.h"
#include "java/util/Formatter.h"

namespace java {

java::String
Formatter::appendText(const java::String &left, const java::String &right) {
    const char *leftRaw = left.toCString();
    const char *rightRaw = right.toCString();
    const std::size_t leftLength = std::strlen(leftRaw);
    const std::size_t rightLength = std::strlen(rightRaw);

    char *joined = new char[leftLength + rightLength + 1];
    std::memcpy(joined, leftRaw, leftLength);
    std::memcpy(joined + leftLength, rightRaw, rightLength);
    joined[leftLength + rightLength] = '\0';

    java::String result(joined);
    delete[] joined;
    return result;
}

java::String
Formatter::formatToString(const char *formatText, va_list arguments) {
    if ( formatText == nullptr ) {
        return java::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), formatText, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    Formatter::vformat(dynamicBuffer, required + 1, formatText, argumentsCopy);
    va_end(argumentsCopy);

    java::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

Formatter::Formatter():
    outputStream(nullptr),
    content(),
    closed(false)
{
}

Formatter::Formatter(java::OutputStream *outputStream):
    outputStream(outputStream),
    content(),
    closed(false)
{
}

Formatter::~Formatter() {
    close();
}

java::OutputStream *
Formatter::out() const {
    return outputStream;
}

void
Formatter::flush() {
    if ( closed || outputStream == nullptr ) {
        return;
    }
    outputStream->flush();
}

void
Formatter::close() {
    if ( closed ) {
        return;
    }
    flush();
    outputStream = nullptr;
    closed = true;
}

Formatter &
Formatter::format(const char *formatText, ...) {
    if ( closed || formatText == nullptr ) {
        return *this;
    }

    va_list arguments;
    va_start(arguments, formatText);
    const java::String text = formatToString(formatText, arguments);
    va_end(arguments);

    content = appendText(content, text);
    if ( outputStream != nullptr && !text.isEmpty() ) {
        outputStream->write(reinterpret_cast<const unsigned char *>(text.toCString()), 0, text.length());
    }
    return *this;
}

int
Formatter::format(char *buffer, int bufferSize, const char *formatText, ...) {
    va_list arguments;
    va_start(arguments, formatText);
    const int written = vformat(buffer, bufferSize, formatText, arguments);
    va_end(arguments);
    return written;
}

int
Formatter::vformat(char *buffer, int bufferSize, const char *formatText, va_list arguments) {
    if ( buffer == nullptr || bufferSize <= 0 || formatText == nullptr ) {
        return -1;
    }

    const int written = std::vsnprintf(buffer, static_cast<std::size_t>(bufferSize), formatText, arguments);
    if ( written < 0 ) {
        buffer[0] = '\0';
    }
    return written;
}

}
