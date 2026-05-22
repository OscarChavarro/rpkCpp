#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "java/io/OutputStream.h"
#include "java/util/Formatter.h"

#ifndef va_copy
#if defined(__va_copy)
#define va_copy(dst, src) __va_copy((dst), (src))
#else
#define va_copy(dst, src) ((dst) = (src))
#endif
#endif


String
Formatter::appendText(const String &left, const String &right) {
    const char *leftRaw = left.toCString();
    const char *rightRaw = right.toCString();
    const size_t leftLength = strlen(leftRaw);
    const size_t rightLength = strlen(rightRaw);

    char *joined = new char[leftLength + rightLength + 1];
    memcpy(joined, leftRaw, leftLength);
    memcpy(joined + leftLength, rightRaw, rightLength);
    joined[leftLength + rightLength] = '\0';

    String result(joined);
    delete[] joined;
    return result;
}

String
Formatter::formatToString(const char *formatText, va_list arguments) {
    if ( formatText == NULL ) {
        return String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(localBuffer, ((int)(sizeof(localBuffer))), formatText, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return String();
    }
    if ( required < ((int)(sizeof(localBuffer))) ) {
        return String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    Formatter::vformat(dynamicBuffer, required + 1, formatText, argumentsCopy);
    va_end(argumentsCopy);

    String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

Formatter::Formatter():
    outputStream(NULL),
    content(),
    closed(false)
{
}

Formatter::Formatter(OutputStream *outputStream):
    outputStream(outputStream),
    content(),
    closed(false)
{
}

Formatter::~Formatter() {
    close();
}

OutputStream *
Formatter::out() const {
    return outputStream;
}

void
Formatter::flush() {
    if ( closed || outputStream == NULL ) {
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
    outputStream = NULL;
    closed = true;
}

Formatter &
Formatter::format(const char *formatText, ...) {
    if ( closed || formatText == NULL ) {
        return *this;
    }

    va_list arguments;
    va_start(arguments, formatText);
    const String text = formatToString(formatText, arguments);
    va_end(arguments);

    content = appendText(content, text);
    if ( outputStream != NULL && !text.isEmpty() ) {
        outputStream->write(((const unsigned char *)(text.toCString())), 0, text.length());
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
    if ( buffer == NULL || bufferSize <= 0 || formatText == NULL ) {
        return -1;
    }

    const int written = vsnprintf(buffer, ((size_t)(bufferSize)), formatText, arguments);
    if ( written < 0 ) {
        buffer[0] = '\0';
    }
    return written;
}
