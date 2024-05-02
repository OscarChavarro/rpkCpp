/**
Routines for recognizing and moving about words in strings.
*/

#include <cctype>
#include <cstring>

#include "io/mgf/words.h"

/**
Skip integer in string
*/
static char *
isSkipWords(char *s)
{
    while ( isspace(*s) ) {
        s++;
    }
    if ( *s == '-' || *s == '+' ) {
        s++;
    }
    if ( !isdigit(*s) ) {
        return nullptr;
    }
    do {
        s++;
    } while (isdigit(*s) );
    return s;
}

/**
Skip float in string
*/
static char *
fileSkipWords(char *s)
{
    char *cp;

    while ( isspace(*s) ) {
        s++;
    }
    if ( *s == '-' || *s == '+' ) {
        s++;
    }
    cp = s;
    while ( isdigit(*cp) ) {
        cp++;
    }
    if ( *cp == '.' ) {
        cp++;
        s++;
        while ( isdigit(*cp) ) {
            cp++;
        }
    }
    if ( cp == s ) {
        return nullptr;
    }
    if ( *cp == 'e' || *cp == 'E' ) {
        return isSkipWords(cp + 1);
    }
    return cp;
}

/**
Check integer format
*/
int
isIntWords(char *s)
{
    const char *cp = isSkipWords(s);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
isIntDWords(char *s, char *ds)
{
    const char *cp = isSkipWords(s);
    return cp != nullptr && strchr(ds, *cp) != nullptr;
}

/**
Check float format
*/
int
isFloatWords(char *s)
{
    const char *cp = fileSkipWords(s);
    return cp != nullptr && *cp == '\0';
}

/**
Check integer format with delimiter set
*/
int
isFloatDWords(char *s, char *ds)
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
    while ( *s == '_' ) {
        // skip leading underscores
        s++;
    }
    if ( !isascii(*s) || !isalpha(*s) ) {
        // start with a letter
        return 0;
    }
    while ( isascii(*++s) && isgraph(*s) ) {
        // all visible characters
    }
    return *s == '\0'; // ending in nul
}
