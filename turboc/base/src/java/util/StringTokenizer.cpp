#include <string.h>

#include "java/util/StringTokenizer.h"


StringTokenizer::StringTokenizer(const String &text, const char *delimiters):
    text(NULL),
    delimiters(NULL),
    cursor(0)
{
    const char *sourceText = text.toCString();
    const char *sourceDelimiters = delimiters == NULL ? " \t\n\r\f" : delimiters;

    this->text = new char[strlen(sourceText) + 1];
    strcpy(this->text, sourceText);

    this->delimiters = new char[strlen(sourceDelimiters) + 1];
    strcpy(this->delimiters, sourceDelimiters);
}

StringTokenizer::~StringTokenizer() {
    dispose();
}

void
StringTokenizer::dispose() {
    if ( text != NULL ) {
        delete[] text;
        text = NULL;
    }
    if ( delimiters != NULL ) {
        delete[] delimiters;
        delimiters = NULL;
    }
}

bool
StringTokenizer::isDelimiter(char ch) const {
    if ( delimiters == NULL ) {
        return false;
    }
    return strchr(delimiters, ch) != NULL;
}

int
StringTokenizer::findTokenStart(int from) const {
    if ( text == NULL ) {
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
    if ( text == NULL ) {
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

String
StringTokenizer::nextToken() {
    const int tokenStart = findTokenStart(cursor);
    if ( tokenStart < 0 ) {
        return String();
    }
    const int tokenEnd = findTokenEnd(tokenStart);
    const int sourceLength = ((int)(strlen(text)));
    const int effectiveTokenEnd = (tokenEnd < 0) ? sourceLength : tokenEnd;
    const int tokenLength = effectiveTokenEnd - tokenStart;
    char *token = new char[((size_t)(tokenLength)) + 1];
    for ( int i = 0; i < tokenLength; i++ ) {
        token[i] = text[tokenStart + i];
    }
    token[tokenLength] = '\0';
    cursor = (tokenEnd < 0) ? sourceLength : tokenEnd + 1;
    String result(token);
    delete[] token;
    return result;
}

