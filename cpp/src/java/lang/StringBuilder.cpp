#include <cstring>

#include "java/lang/StringBuilder.h"

namespace java {

void
StringBuilder::ensureCapacity(int requiredLength) {
    if ( requiredLength + 1 <= capacity ) {
        return;
    }
    int newCapacity = capacity <= 0 ? 16 : capacity;
    while ( newCapacity < requiredLength + 1 ) {
        newCapacity *= 2;
    }
    char *newValue = new char[static_cast<std::size_t>(newCapacity)];
    if ( value == nullptr ) {
        newValue[0] = '\0';
    } else {
        std::strcpy(newValue, value);
        delete[] value;
    }
    value = newValue;
    capacity = newCapacity;
}

void
StringBuilder::assignFromCString(const char *text) {
    const char *source = text == nullptr ? "" : text;
    const int sourceLength = static_cast<int>(std::strlen(source));
    ensureCapacity(sourceLength);
    std::strcpy(value, source);
    lengthValue = sourceLength;
}

StringBuilder::StringBuilder():
    value(nullptr),
    lengthValue(0),
    capacity(0)
{
    ensureCapacity(0);
    value[0] = '\0';
}

StringBuilder::StringBuilder(const StringBuilder &other):
    value(nullptr),
    lengthValue(0),
    capacity(0)
{
    assignFromCString(other.value);
}

StringBuilder::StringBuilder(const String &text):
    value(nullptr),
    lengthValue(0),
    capacity(0)
{
    assignFromCString(text.toCString());
}

StringBuilder::~StringBuilder() {
    dispose();
}

void
StringBuilder::dispose() {
    if ( value != nullptr ) {
        delete[] value;
        value = nullptr;
    }
    lengthValue = 0;
    capacity = 0;
}

StringBuilder &
StringBuilder::operator=(const StringBuilder &other) {
    if ( this != &other ) {
        assignFromCString(other.value);
    }
    return *this;
}

int
StringBuilder::length() const {
    return lengthValue;
}

char
StringBuilder::charAt(int index) const {
    if ( index < 0 || index >= lengthValue ) {
        return '\0';
    }
    return value[index];
}

void
StringBuilder::clear() {
    lengthValue = 0;
    if ( value != nullptr ) {
        value[0] = '\0';
    }
}

StringBuilder &
StringBuilder::append(const String &text) {
    return append(text.toCString());
}

StringBuilder &
StringBuilder::append(const char *text) {
    if ( text == nullptr ) {
        return *this;
    }
    const int textLength = static_cast<int>(std::strlen(text));
    if ( textLength <= 0 ) {
        return *this;
    }
    ensureCapacity(lengthValue + textLength);
    std::strcat(value, text);
    lengthValue += textLength;
    return *this;
}

StringBuilder &
StringBuilder::append(const char *text, int textLength) {
    if ( text == nullptr || textLength <= 0 ) {
        return *this;
    }
    ensureCapacity(lengthValue + textLength);
    std::strncat(value, text, static_cast<std::size_t>(textLength));
    lengthValue += textLength;
    value[lengthValue] = '\0';
    return *this;
}

StringBuilder &
StringBuilder::append(char ch) {
    ensureCapacity(lengthValue + 1);
    value[lengthValue] = ch;
    lengthValue++;
    value[lengthValue] = '\0';
    return *this;
}

String
StringBuilder::toString() const {
    return String(value);
}

}
