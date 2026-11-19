#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/Formatter.h"
#include "vsdk/toolkit/common/Cpp98Support.h"

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

String &
String::operator=(const char *other) {
    assignText(other);
    return *this;
}

const char *
String::toCString() const {
    return value == NULL ? "" : value;
}

const char *
String::c_str() const {
    return toCString();
}

const char *
String::data() const {
    return toCString();
}

int
String::length() const {
    return ((int)(strlen(toCString())));
}

int
String::size() const {
    return length();
}

BOOLEAN
String::isEmpty() const {
    return toCString()[0] == '\0';
}

BOOLEAN
String::empty() const {
    return isEmpty();
}

char
String::charAt(int index) const {
    if ( index < 0 || index >= length() ) {
        return '\0';
    }
    return toCString()[index];
}

char
String::operator[](int index) const {
    return charAt(index);
}

char &
String::operator[](int index) {
    static char nullChar = '\0';
    if ( index < 0 || index >= length() ) {
        return nullChar;
    }
    return value[index];
}

char *
String::begin() {
    return value == NULL ? NULL : value;
}

char *
String::end() {
    return value == NULL ? NULL : (value + length());
}

const char *
String::begin() const {
    return toCString();
}

const char *
String::end() const {
    return toCString() + length();
}

BOOLEAN
String::equals(const String &other) const {
    return strcmp(toCString(), other.toCString()) == 0;
}

BOOLEAN
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

String
String::substr(int beginIndex) const {
    return substring(beginIndex);
}

String
String::substr(int beginIndex, int count) const {
    if ( count <= 0 ) {
        return String();
    }
    return substring(beginIndex, beginIndex + count);
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

int
String::find(char token, int fromIndex) const {
    return indexOf(token, fromIndex);
}

int
String::find_last_of(const char *tokens) const {
    if ( tokens == NULL || tokens[0] == '\0' ) {
        return -1;
    }
    const char *source = toCString();
    for ( int i = length() - 1; i >= 0; --i ) {
        for ( int j = 0; tokens[j] != '\0'; ++j ) {
            if ( source[i] == tokens[j] ) {
                return i;
            }
        }
    }
    return -1;
}

String &
String::erase(int index, int count) {
    const int sourceLength = length();
    if ( index < 0 || index >= sourceLength || count <= 0 ) {
        return *this;
    }
    if ( index + count > sourceLength ) {
        count = sourceLength - index;
    }
    char *buffer = new char[((size_t)(sourceLength - count)) + 1];
    int out = 0;
    for ( int i = 0; i < sourceLength; ++i ) {
        if ( i < index || i >= index + count ) {
            buffer[out++] = toCString()[i];
        }
    }
    buffer[out] = '\0';
    assignText(buffer);
    delete[] buffer;
    return *this;
}

BOOLEAN
String::startsWith(const char *prefix) const {
    if ( prefix == NULL ) {
        return FALSE;
    }
    const size_t prefixLength = strlen(prefix);
    const size_t sourceLength = strlen(toCString());
    if ( prefixLength > sourceLength ) {
        return FALSE;
    }
    return strncmp(toCString(), prefix, prefixLength) == 0;
}

int
String::rfind(char token) const {
    const char *source = toCString();
    for ( int i = length() - 1; i >= 0; i-- ) {
        if ( source[i] == token ) {
            return i;
        }
    }
    return -1;
}

int
String::rfind(const char *token) const {
    if ( token == NULL || token[0] == '\0' ) {
        return length();
    }
    const int tokenLength = ((int)(strlen(token)));
    const int sourceLength = length();
    if ( tokenLength > sourceLength ) {
        return -1;
    }
    const char *source = toCString();
    for ( int i = sourceLength - tokenLength; i >= 0; i-- ) {
        if ( strncmp(source + i, token, ((size_t)(tokenLength))) == 0 ) {
            return i;
        }
    }
    return -1;
}

int
String::rfind(const char *token, int fromIndex) const {
    if ( token == NULL || token[0] == '\0' ) {
        return length();
    }
    const int tokenLength = ((int)(strlen(token)));
    const int sourceLength = length();
    if ( tokenLength > sourceLength ) {
        return -1;
    }
    int start = sourceLength - tokenLength;
    if ( fromIndex >= 0 && fromIndex < start ) {
        start = fromIndex;
    }
    const char *source = toCString();
    for ( int i = start; i >= 0; i-- ) {
        if ( strncmp(source + i, token, ((size_t)(tokenLength))) == 0 ) {
            return i;
        }
    }
    return -1;
}

int
String::rfind(const String &token) const {
    return rfind(token.toCString());
}

String
String::concat(const String &other) const {
    return concat(other.toCString());
}

String
String::concat(const char *other) const {
    const char *left = toCString();
    const char *right = other == NULL ? "null" : other;

    const size_t leftLength = strlen(left);
    const size_t rightLength = strlen(right);
    char *joined = new char[leftLength + rightLength + 1];
    memcpy(joined, left, leftLength);
    memcpy(joined + leftLength, right, rightLength);
    joined[leftLength + rightLength] = '\0';

    String result(joined);
    delete[] joined;
    return result;
}

String
String::operator+(const String &other) const {
    return concat(other);
}

String
String::operator+(const char *other) const {
    return concat(other);
}

String &
String::operator+=(const String &other) {
    *this = concat(other);
    return *this;
}

String &
String::operator+=(const char *other) {
    *this = concat(other);
    return *this;
}

BOOLEAN
String::operator==(const String &other) const {
    return equals(other);
}

BOOLEAN
String::operator==(const char *other) const {
    return equals(other);
}

BOOLEAN
String::operator!=(const String &other) const {
    return !equals(other);
}

BOOLEAN
String::operator!=(const char *other) const {
    return !equals(other);
}

BOOLEAN
String::operator<(const String &other) const {
    return strcmp(toCString(), other.toCString()) < 0;
}

BOOLEAN
String::operator<=(const String &other) const {
    return strcmp(toCString(), other.toCString()) <= 0;
}

BOOLEAN
String::operator>(const String &other) const {
    return strcmp(toCString(), other.toCString()) > 0;
}

BOOLEAN
String::operator>=(const String &other) const {
    return strcmp(toCString(), other.toCString()) >= 0;
}

#ifdef VSDK_HAS_LONG_LONG
String
String::valueOf(long long value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%lld", value);
    return String(buffer);
}

String
String::valueOf(unsigned long long value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%llu", value);
    return String(buffer);
}
#endif

String
String::valueOf(long value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    return String(buffer);
}

String
String::valueOf(unsigned long value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%lu", value);
    return String(buffer);
}

String
String::valueOf(int value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return String(buffer);
}

String
String::valueOf(unsigned int value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%u", value);
    return String(buffer);
}

String
String::valueOf(double value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%g", value);
    return String(buffer);
}

String
String::valueOf(float value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%g", ((double)(value)));
    return String(buffer);
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

int
String::indexOf(const String &text, int fromIndex) const {
    const char *source = toCString();
    const int sourceLength = length();
    if ( fromIndex < 0 ) {
        fromIndex = 0;
    }
    if ( fromIndex > sourceLength ) {
        return -1;
    }
    const char *found = strstr(source + fromIndex, text.toCString());
    return found == NULL ? -1 : ((int)(found - source));
}

BOOLEAN
String::contains(const String &text) const {
    return indexOf(text) >= 0;
}

BOOLEAN
String::endsWith(const String &suffix) const {
    const int sourceLength = length();
    const int suffixLength = suffix.length();
    return suffixLength <= sourceLength &&
        strcmp(toCString() + sourceLength - suffixLength,
                    suffix.toCString()) == 0;
}

String
String::replace(const String &target, const String &replacement) const {
    const int targetLength = target.length();
    if ( targetLength == 0 ) {
        return *this;
    }
    String result;
    int start = 0;
    int found;
    while ( (found = indexOf(target, start)) >= 0 ) {
        result += substring(start, found);
        result += replacement;
        start = found + targetLength;
    }
    result += substring(start);
    return result;
}

String
String::toUpperCase() const {
    String result(*this);
    for ( int i = 0; i < result.length(); i++ ) {
        char c = result.value[i];
        if ( c >= 'a' && c <= 'z' ) {
            result.value[i] = ((char)(c - 'a' + 'A'));
        }
    }
    return result;
}

String
String::toLowerCase() const {
    String result(*this);
    for ( int i = 0; i < result.length(); i++ ) {
        char c = result.value[i];
        if ( c >= 'A' && c <= 'Z' ) {
            result.value[i] = ((char)(c - 'A' + 'a'));
        }
    }
    return result;
}

BOOLEAN
String::equalsIgnoreCase(const String &other) const {
    return toLowerCase().equals(other.toLowerCase());
}

String
String::trim() const {
    const char *source = toCString();
    int begin = 0;
    int end = length();
    while ( begin < end &&
            ((unsigned char)(source[begin])) <= ' ' ) {
        begin++;
    }
    while ( end > begin &&
            ((unsigned char)(source[end - 1])) <= ' ' ) {
        end--;
    }
    return substring(begin, end);
}

String
operator+(const char *left, const String &right) {
    return String(left).concat(right);
}

// Integral constants bound to references (i.e. by `?:`) need a
// definition out of the class in C++98
