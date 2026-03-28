#include "java/util/Formatter.h"

#include <cstdio>
#include <cstring>

#include "java/io/OutputStream.h"

namespace java {
namespace util {

namespace {
static java::lang::String
appendText(const java::lang::String &left, const java::lang::String &right) {
    const char *leftRaw = left.toCString();
    const char *rightRaw = right.toCString();
    const std::size_t leftLength = std::strlen(leftRaw);
    const std::size_t rightLength = std::strlen(rightRaw);

    char *joined = new char[leftLength + rightLength + 1];
    std::memcpy(joined, leftRaw, leftLength);
    std::memcpy(joined + leftLength, rightRaw, rightLength);
    joined[leftLength + rightLength] = '\0';

    java::lang::String result(joined);
    delete[] joined;
    return result;
}
}

int
vformatToBuffer(char *buffer, int bufferSize, const char *formatText, va_list arguments) {
    if ( buffer == nullptr || bufferSize <= 0 || formatText == nullptr ) {
        return -1;
    }
    const int written = std::vsnprintf(buffer, static_cast<std::size_t>(bufferSize), formatText, arguments);
    if ( written < 0 ) {
        buffer[0] = '\0';
    }
    return written;
}

int
formatToBuffer(char *buffer, int bufferSize, const char *formatText, ...) {
    va_list arguments;
    va_start(arguments, formatText);
    const int written = vformatToBuffer(buffer, bufferSize, formatText, arguments);
    va_end(arguments);
    return written;
}

java::lang::String
vformat(const char *formatText, va_list arguments) {
    if ( formatText == nullptr ) {
        return java::lang::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = std::vsnprintf(localBuffer, sizeof(localBuffer), formatText, argumentsCopy);
    va_end(argumentsCopy);

    if ( required <= 0 ) {
        return java::lang::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::lang::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    std::vsnprintf(dynamicBuffer, static_cast<std::size_t>(required + 1), formatText, argumentsCopy);
    va_end(argumentsCopy);

    java::lang::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

java::lang::String
format(const char *formatText, ...) {
    va_list arguments;
    va_start(arguments, formatText);
    java::lang::String result = vformat(formatText, arguments);
    va_end(arguments);
    return result;
}

Formatter::Formatter():
    outputStream(nullptr),
    content(),
    closed(false)
{
}

Formatter::Formatter(java::io::OutputStream *outputStream):
    outputStream(outputStream),
    content(),
    closed(false)
{
}

Formatter::~Formatter() {
    close();
}

java::io::OutputStream *
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
    const java::lang::String text = vformat(formatText, arguments);
    va_end(arguments);

    content = appendText(content, text);
    if ( outputStream != nullptr && !text.isEmpty() ) {
        outputStream->write(reinterpret_cast<const unsigned char *>(text.toCString()), 0, text.length());
    }
    return *this;
}

}
}
