#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "java/lang/String.h"
#include "java/util/Formatter.h"
namespace java {

void
String::assignText(const char *text) {
    const char *source = (text == nullptr) ? "" : text;
    const std::size_t textLength = std::strlen(source);
    char *newValue = new char[textLength + 1];
    std::strcpy(newValue, source);
    if ( value != nullptr ) {
        delete[] value;
    }
    value = newValue;
}

String::String():
    value(nullptr)
{
    assignText("");
}

String::String(const String &other):
    value(nullptr)
{
    assignText(other.value);
}

String::String(const char *text):
    value(nullptr)
{
    assignText(text);
}

String::~String() {
    dispose();
}

void
String::dispose() {
    if ( value != nullptr ) {
        delete[] value;
        value = nullptr;
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
    return value == nullptr ? "" : value;
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
    return static_cast<int>(std::strlen(toCString()));
}

int
String::size() const {
    return length();
}

bool
String::isEmpty() const {
    return toCString()[0] == '\0';
}

bool
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
    return value == nullptr ? nullptr : value;
}

char *
String::end() {
    return value == nullptr ? nullptr : (value + length());
}

const char *
String::begin() const {
    return toCString();
}

const char *
String::end() const {
    return toCString() + length();
}

bool
String::equals(const String &other) const {
    return std::strcmp(toCString(), other.toCString()) == 0;
}

bool
String::equals(const char *other) const {
    return std::strcmp(toCString(), other == nullptr ? "" : other) == 0;
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
    char *slice = new char[static_cast<std::size_t>(sliceLength) + 1];
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
    if ( tokens == nullptr || tokens[0] == '\0' ) {
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
    char *buffer = new char[static_cast<std::size_t>(sourceLength - count) + 1];
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

bool
String::startsWith(const char *prefix) const {
    if ( prefix == nullptr ) {
        return false;
    }
    const std::size_t prefixLength = std::strlen(prefix);
    const std::size_t sourceLength = std::strlen(toCString());
    if ( prefixLength > sourceLength ) {
        return false;
    }
    return std::strncmp(toCString(), prefix, prefixLength) == 0;
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
    if ( token == nullptr || token[0] == '\0' ) {
        return length();
    }
    const int tokenLength = static_cast<int>(std::strlen(token));
    const int sourceLength = length();
    if ( tokenLength > sourceLength ) {
        return -1;
    }
    const char *source = toCString();
    for ( int i = sourceLength - tokenLength; i >= 0; i-- ) {
        if ( std::strncmp(source + i, token, static_cast<std::size_t>(tokenLength)) == 0 ) {
            return i;
        }
    }
    return -1;
}

int
String::rfind(const char *token, int fromIndex) const {
    if ( token == nullptr || token[0] == '\0' ) {
        return length();
    }
    const int tokenLength = static_cast<int>(std::strlen(token));
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
        if ( std::strncmp(source + i, token, static_cast<std::size_t>(tokenLength)) == 0 ) {
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
    const char *right = other == nullptr ? "null" : other;

    const std::size_t leftLength = std::strlen(left);
    const std::size_t rightLength = std::strlen(right);
    char *joined = new char[leftLength + rightLength + 1];
    std::memcpy(joined, left, leftLength);
    std::memcpy(joined + leftLength, right, rightLength);
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

String
String::operator+(char other) const {
    const char text[2] = { other, '\0' };
    return concat(text);
}

String &
String::operator+=(char other) {
    const char text[2] = { other, '\0' };
    *this = concat(text);
    return *this;
}

bool
String::operator==(const String &other) const {
    return equals(other);
}

bool
String::operator==(const char *other) const {
    return equals(other);
}

bool
String::operator!=(const String &other) const {
    return !equals(other);
}

bool
String::operator!=(const char *other) const {
    return !equals(other);
}

bool
String::operator<(const String &other) const {
    return std::strcmp(toCString(), other.toCString()) < 0;
}

bool
String::operator<=(const String &other) const {
    return std::strcmp(toCString(), other.toCString()) <= 0;
}

bool
String::operator>(const String &other) const {
    return std::strcmp(toCString(), other.toCString()) > 0;
}

bool
String::operator>=(const String &other) const {
    return std::strcmp(toCString(), other.toCString()) >= 0;
}

String
String::valueOf(long long value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%lld", value);
    return String(buffer);
}

String
String::valueOf(unsigned long long value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%llu", value);
    return String(buffer);
}

String
String::valueOf(long value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%ld", value);
    return String(buffer);
}

String
String::valueOf(unsigned long value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%lu", value);
    return String(buffer);
}

String
String::valueOf(int value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%d", value);
    return String(buffer);
}

String
String::valueOf(unsigned int value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%u", value);
    return String(buffer);
}

String
String::valueOf(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%g", value);
    return String(buffer);
}

String
String::valueOf(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%g", static_cast<double>(value));
    return String(buffer);
}

String
String::format(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    String result = formatCStringToJavaString(format, arguments);
    va_end(arguments);
    return result;
}

String
String::formatCStringToJavaString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::Formatter::vformat(
        localBuffer,
        static_cast<int>(sizeof(localBuffer)),
        format,
        argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

bool
String::startsWith(const char *prefix, int offset) const {
    if ( prefix == nullptr || offset < 0 ) {
        return false;
    }
    const std::size_t prefixLength = std::strlen(prefix);
    const std::size_t sourceLength = std::strlen(toCString());
    if ( static_cast<std::size_t>(offset) > sourceLength ||
         prefixLength > sourceLength - static_cast<std::size_t>(offset) ) {
        return false;
    }
    return std::strncmp(toCString() + offset, prefix, prefixLength) == 0;
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
    const char *found = std::strstr(source + fromIndex, text.toCString());
    return found == nullptr ? -1 : static_cast<int>(found - source);
}

bool
String::contains(const String &text) const {
    return indexOf(text) >= 0;
}

bool
String::endsWith(const String &suffix) const {
    const int sourceLength = length();
    const int suffixLength = suffix.length();
    return suffixLength <= sourceLength &&
        std::strcmp(toCString() + sourceLength - suffixLength,
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
            result.value[i] = static_cast<char>(c - 'a' + 'A');
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
            result.value[i] = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

bool
String::equalsIgnoreCase(const String &other) const {
    return toLowerCase().equals(other.toLowerCase());
}

String
String::trim() const {
    const char *source = toCString();
    int begin = 0;
    int end = length();
    while ( begin < end &&
            static_cast<unsigned char>(source[begin]) <= ' ' ) {
        begin++;
    }
    while ( end > begin &&
            static_cast<unsigned char>(source[end - 1]) <= ' ' ) {
        end--;
    }
    return substring(begin, end);
}

String
operator+(const char *left, const String &right) {
    return String(left).concat(right);
}

}
