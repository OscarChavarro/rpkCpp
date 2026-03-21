#include "java/util/StringTokenizer.h"

#include <cstring>

namespace java {
namespace util {

StringTokenizer::StringTokenizer(const java::lang::String &text, const char *delimiters):
    text(nullptr),
    delimiters(nullptr),
    cursor(0)
{
    const char *sourceText = text.toCString();
    const char *sourceDelimiters = delimiters == nullptr ? " \t\n\r\f" : delimiters;

    this->text = new char[std::strlen(sourceText) + 1];
    std::strcpy(this->text, sourceText);

    this->delimiters = new char[std::strlen(sourceDelimiters) + 1];
    std::strcpy(this->delimiters, sourceDelimiters);
}

StringTokenizer::~StringTokenizer() {
    dispose();
}

void
StringTokenizer::dispose() {
    if ( text != nullptr ) {
        delete[] text;
        text = nullptr;
    }
    if ( delimiters != nullptr ) {
        delete[] delimiters;
        delimiters = nullptr;
    }
}

bool
StringTokenizer::isDelimiter(char ch) const {
    if ( delimiters == nullptr ) {
        return false;
    }
    return std::strchr(delimiters, ch) != nullptr;
}

int
StringTokenizer::findTokenStart(int from) const {
    if ( text == nullptr ) {
        return -1;
    }
    int index = from < 0 ? 0 : from;
    while ( text[index] != '\0' && isDelimiter(text[index]) ) {
        index++;
    }
    if ( text[index] == '\0' ) {
        return -1;
    }
    return index;
}

int
StringTokenizer::findTokenEnd(int from) const {
    if ( text == nullptr ) {
        return -1;
    }
    int index = from < 0 ? 0 : from;
    while ( text[index] != '\0' && !isDelimiter(text[index]) ) {
        index++;
    }
    if ( text[index] == '\0' ) {
        return -1;
    }
    return index;
}

bool
StringTokenizer::hasMoreTokens() const {
    return findTokenStart(cursor) >= 0;
}

java::lang::String
StringTokenizer::nextToken() {
    const int tokenStart = findTokenStart(cursor);
    if ( tokenStart < 0 ) {
        return java::lang::String();
    }
    const int tokenEnd = findTokenEnd(tokenStart);
    if ( tokenEnd < 0 ) {
        cursor = static_cast<int>(std::strlen(text));
        return java::lang::String(text + tokenStart);
    }
    const int tokenLength = tokenEnd - tokenStart;
    char *token = new char[static_cast<std::size_t>(tokenLength) + 1];
    std::strncpy(token, text + tokenStart, static_cast<std::size_t>(tokenLength));
    token[tokenLength] = '\0';
    cursor = tokenEnd + 1;
    java::lang::String result(token);
    delete[] token;
    return result;
}

}
}
