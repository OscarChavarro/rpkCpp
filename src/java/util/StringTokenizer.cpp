#include <cstring>

#include "java/util/StringTokenizer.h"

namespace java {

StringTokenizer::StringTokenizer(const java::String &text, const char *delimiters):
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

java::String
StringTokenizer::nextToken() {
    const int tokenStart = findTokenStart(cursor);
    if ( tokenStart < 0 ) {
        return java::String();
    }
    const int tokenEnd = findTokenEnd(tokenStart);
    const int sourceLength = static_cast<int>(std::strlen(text));
    const int effectiveTokenEnd = (tokenEnd < 0) ? sourceLength : tokenEnd;
    const int tokenLength = effectiveTokenEnd - tokenStart;
    char *token = new char[static_cast<std::size_t>(tokenLength) + 1];
    for ( int i = 0; i < tokenLength; i++ ) {
        token[i] = text[tokenStart + i];
    }
    token[tokenLength] = '\0';
    cursor = (tokenEnd < 0) ? sourceLength : tokenEnd + 1;
    java::String result(token);
    delete[] token;
    return result;
}

}
