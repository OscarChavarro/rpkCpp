#include <stdarg.h>
#include <string.h>

#include "java/lang/String.h"
#include "java/util/Formatter.h"


void
String::assignText(const char *text) {
    const char *source = (text == NULL) ? "" : text;
    const size_t textLength = strlen(source);
    char *newValue = new char[textLength + 1];
    strcpy(newValue, source);
    if ( value != NULL ) {
        delete[] value;
    }
    value = newValue;
}

String::String():
    value(NULL)
{
    assignText("");
}

String::String(const String &other):
    value(NULL)
{
    assignText(other.value);
}

String::String(const char *text):
    value(NULL)
{
    assignText(text);
}

String::~String() {
    dispose();
}

void
String::dispose() {
    if ( value != NULL ) {
        delete[] value;
        value = NULL;
    }
}

String &
String::operator=(const String &other) {
    if ( this != &other ) {
        assignText(other.value);
    }
    return *this;
}

const char *
String::toCString() const {
    return value == NULL ? "" : value;
}

int
String::length() const {
    return ((int)(strlen(toCString())));
}

bool
String::isEmpty() const {
    return toCString()[0] == '\0';
}

char
String::charAt(int index) const {
    if ( index < 0 || index >= length() ) {
        return '\0';
    }
    return toCString()[index];
}

bool
String::equals(const String &other) const {
    return strcmp(toCString(), other.toCString()) == 0;
}

bool
String::equals(const char *other) const {
    return strcmp(toCString(), other == NULL ? "" : other) == 0;
}

String
String::substring(int beginIndex) const {
    if ( beginIndex <= 0 ) {
        return String(toCString());
    }
    const int sourceLength = length();
    if ( beginIndex >= sourceLength ) {
        return String();
    }
    return substring(beginIndex, sourceLength);
}

String
String::substring(int beginIndex, int endIndex) const {
    const char *source = toCString();
    const int sourceLength = length();
    if ( beginIndex < 0 ) {
        beginIndex = 0;
    }
    if ( endIndex > sourceLength ) {
        endIndex = sourceLength;
    }
    if ( beginIndex >= sourceLength || endIndex <= beginIndex ) {
        return String();
    }
    const int sliceLength = endIndex - beginIndex;
    char *slice = new char[((size_t)(sliceLength)) + 1];
    for ( int i = 0; i < sliceLength; i++ ) {
        slice[i] = source[beginIndex + i];
    }
    slice[sliceLength] = '\0';
    String result(slice);
    delete[] slice;
    return result;
}

int
String::indexOf(char token, int fromIndex) const {
    if ( fromIndex < 0 ) {
        fromIndex = 0;
    }
    const char *source = toCString();
    const int sourceLength = length();
    if ( fromIndex >= sourceLength ) {
        return -1;
    }
    for ( int i = fromIndex; i < sourceLength; i++ ) {
        if ( source[i] == token ) {
            return i;
        }
    }
    return -1;
}

bool
String::startsWith(const char *prefix) const {
    if ( prefix == NULL ) {
        return false;
    }
    const size_t prefixLength = strlen(prefix);
    const size_t sourceLength = strlen(toCString());
    if ( prefixLength > sourceLength ) {
        return false;
    }
    return strncmp(toCString(), prefix, prefixLength) == 0;
}

String
String::formatCStringToJavaString(const char *format, va_list arguments) {
    if ( format == NULL ) {
        return String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = Formatter::vformat(
        localBuffer,
        ((int)(sizeof(localBuffer))),
        format,
        argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return String();
    }
    if ( required < ((int)(sizeof(localBuffer))) ) {
        return String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

