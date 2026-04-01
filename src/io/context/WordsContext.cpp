#include <cstring>

#include "java/lang/Character.h"
#include "io/context/WordsContext.h"

bool
WordsContext::isAsciiCode(int value) {
    return value >= 0 && value <= 127;
}

bool
WordsContext::isAsciiGraph(int value) {
    return value >= 33 && value <= 126;
}

/**
Skip integer in string
*/
const char *
WordsContext::skipInt(const char *text)
{
    int index = 0;
    while ( java::Character::isSpace(text[index]) ) {
        index++;
    }
    if ( text[index] == '-' || text[index] == '+' ) {
        index++;
    }
    if ( !java::Character::isDigit(text[index]) ) {
        return nullptr;
    }
    do {
        index++;
    } while (java::Character::isDigit(text[index]) );
    return &text[index];
}

/**
Skip float in string
*/
const char *
WordsContext::skipFloat(const char *text)
{
    int startIndex = 0;
    while ( java::Character::isSpace(text[startIndex]) ) {
        startIndex++;
    }
    if ( text[startIndex] == '-' || text[startIndex] == '+' ) {
        startIndex++;
    }
    int currentIndex = startIndex;
    while ( java::Character::isDigit(text[currentIndex]) ) {
        currentIndex++;
    }
    if ( text[currentIndex] == '.' ) {
        currentIndex++;
        startIndex++;
        while ( java::Character::isDigit(text[currentIndex]) ) {
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
bool
WordsContext::isInt(const char *text)
{
    const char *cp = skipInt(text);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
bool
WordsContext::isIntDelimited(const char *text, const char *delimiters)
{
    const char *cp = skipInt(text);
    return cp != nullptr && strchr(delimiters, *cp) != nullptr;
}

/**
Check float format
*/
bool
WordsContext::isFloat(const char *text)
{
    const char *cp = skipFloat(text);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
bool
WordsContext::isFloatDelimited(const char *text, const char *delimiters)
{
    const char *cp = skipFloat(text);
    return cp != nullptr && strchr(delimiters, *cp) != nullptr;
}

/**
Check for legal identifier name
*/
bool
WordsContext::isName(const char *text)
{
    int index = 0;
    while ( text[index] == '_' ) {
        // skip leading underscores
        index++;
    }
    if ( !WordsContext::isAsciiCode(text[index]) || !java::Character::isLetter(text[index]) ) {
        // start with a letter
        return false;
    }
    int tokenIndex = index + 1;
    while ( WordsContext::isAsciiCode(text[tokenIndex]) && WordsContext::isAsciiGraph(text[tokenIndex]) ) {
        // all visible characters
        tokenIndex++;
    }
    return text[tokenIndex] == '\0'; // ending in nul
}
