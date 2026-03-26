#include <cctype>
#include <cstring>

#include "io/context/WordsContext.h"

/**
Skip integer in string
*/
const char *
WordsContext::skipInt(const char *text)
{
    int index = 0;
    while ( isspace(static_cast<unsigned char>(text[index])) ) {
        index++;
    }
    if ( text[index] == '-' || text[index] == '+' ) {
        index++;
    }
    if ( !isdigit(static_cast<unsigned char>(text[index])) ) {
        return nullptr;
    }
    do {
        index++;
    } while (isdigit(static_cast<unsigned char>(text[index])) );
    return &text[index];
}

/**
Skip float in string
*/
const char *
WordsContext::skipFloat(const char *text)
{
    int startIndex = 0;
    while ( isspace(static_cast<unsigned char>(text[startIndex])) ) {
        startIndex++;
    }
    if ( text[startIndex] == '-' || text[startIndex] == '+' ) {
        startIndex++;
    }
    int currentIndex = startIndex;
    while ( isdigit(static_cast<unsigned char>(text[currentIndex])) ) {
        currentIndex++;
    }
    if ( text[currentIndex] == '.' ) {
        currentIndex++;
        startIndex++;
        while ( isdigit(static_cast<unsigned char>(text[currentIndex])) ) {
            currentIndex++;
        }
    }
    if ( currentIndex == startIndex ) {
        return nullptr;
    }
    if ( text[currentIndex] == 'e' || text[currentIndex] == 'E' ) {
        return skipInt(&text[currentIndex + 1]);
    }
    return &text[currentIndex];
}

/**
Check integer format
*/
int
WordsContext::isInt(const char *text)
{
    const char *cp = skipInt(text);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
WordsContext::isIntDelimited(const char *text, const char *delimiters)
{
    const char *cp = skipInt(text);
    return cp != nullptr && strchr(delimiters, *cp) != nullptr;
}

/**
Check float format
*/
int
WordsContext::isFloat(const char *text)
{
    const char *cp = skipFloat(text);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
WordsContext::isFloatDelimited(const char *text, const char *delimiters)
{
    const char *cp = skipFloat(text);
    return cp != nullptr && strchr(delimiters, *cp) != nullptr;
}

/**
Check for legal identifier name
*/
int
WordsContext::isName(const char *text)
{
    int index = 0;
    while ( text[index] == '_' ) {
        // skip leading underscores
        index++;
    }
    if ( !isascii(static_cast<unsigned char>(text[index])) || !isalpha(static_cast<unsigned char>(text[index])) ) {
        // start with a letter
        return 0;
    }
    int tokenIndex = index + 1;
    while ( isascii(static_cast<unsigned char>(text[tokenIndex])) && isgraph(static_cast<unsigned char>(text[tokenIndex])) ) {
        // all visible characters
        tokenIndex++;
    }
    return text[tokenIndex] == '\0'; // ending in nul
}
