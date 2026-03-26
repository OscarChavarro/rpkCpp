/**
Routines for recognizing and moving about words in strings.
*/

#include <cctype>
#include <cstring>

#include "io/mgf/words.h"

/**
Skip integer in string
*/
static const char *
isSkipWords(const char *s)
{
    int index = 0;
    while ( isspace(static_cast<unsigned char>(s[index])) ) {
        index++;
    }
    if ( s[index] == '-' || s[index] == '+' ) {
        index++;
    }
    if ( !isdigit(static_cast<unsigned char>(s[index])) ) {
        return nullptr;
    }
    do {
        index++;
    } while (isdigit(static_cast<unsigned char>(s[index])) );
    return &s[index];
}

/**
Skip float in string
*/
static const char *
fileSkipWords(const char *s)
{
    int startIndex = 0;
    while ( isspace(static_cast<unsigned char>(s[startIndex])) ) {
        startIndex++;
    }
    if ( s[startIndex] == '-' || s[startIndex] == '+' ) {
        startIndex++;
    }
    int currentIndex = startIndex;
    while ( isdigit(static_cast<unsigned char>(s[currentIndex])) ) {
        currentIndex++;
    }
    if ( s[currentIndex] == '.' ) {
        currentIndex++;
        startIndex++;
        while ( isdigit(static_cast<unsigned char>(s[currentIndex])) ) {
            currentIndex++;
        }
    }
    if ( currentIndex == startIndex ) {
        return nullptr;
    }
    if ( s[currentIndex] == 'e' || s[currentIndex] == 'E' ) {
        return isSkipWords(&s[currentIndex + 1]);
    }
    return &s[currentIndex];
}

/**
Check integer format
*/
int
isIntWords(const char *s)
{
    const char *cp = isSkipWords(s);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
isIntDWords(const char *s, const char *ds)
{
    const char *cp = isSkipWords(s);
    return cp != nullptr && strchr(ds, *cp) != nullptr;
}

/**
Check float format
*/
int
isFloatWords(const char *s)
{
    const char *cp = fileSkipWords(s);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
isFloatDWords(const char *s, const char *ds)
{
    const char *cp = fileSkipWords(s);
    return cp != nullptr && strchr(ds, *cp) != nullptr;
}

/**
Check for legal identifier name
*/
int
isNameWords(const char *s)
{
    int index = 0;
    while ( s[index] == '_' ) {
        // skip leading underscores
        index++;
    }
    if ( !isascii(static_cast<unsigned char>(s[index])) || !isalpha(static_cast<unsigned char>(s[index])) ) {
        // start with a letter
        return 0;
    }
    int tokenIndex = index + 1;
    while ( isascii(static_cast<unsigned char>(s[tokenIndex])) && isgraph(static_cast<unsigned char>(s[tokenIndex])) ) {
        // all visible characters
        tokenIndex++;
    }
    return s[tokenIndex] == '\0'; // ending in nul
}
