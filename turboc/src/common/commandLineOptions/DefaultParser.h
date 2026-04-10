#ifndef __OPTION_CORE_DEFAULT_PARSER__
#define __OPTION_CORE_DEFAULT_PARSER__

#include "common/VSDK.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

template<typename T>
struct DefaultParser {
    static bool parse(const char * /*input*/, T & /*out*/) {
        return false;
    }
};

template<>
struct DefaultParser<int> {
    static bool parse(const char *input, int &out) {
        if ( input == NULL ) {
            return false;
        }
        char *endPointer = NULL;
        const long parsedValue = strtol(input, &endPointer, 10);
        if ( endPointer == input || *endPointer != '\0' ) {
            return false;
        }
        if ( parsedValue < ((long)(INT_MIN)) || parsedValue > ((long)(INT_MAX)) ) {
            return false;
        }
        out = ((int)(parsedValue));
        return true;
    }
};

template<>
struct DefaultParser<long> {
    static bool parse(const char *input, long &out) {
        if ( input == NULL ) {
            return false;
        }
        char *endPointer = NULL;
        const long parsedValue = strtol(input, &endPointer, 10);
        if ( endPointer == input || *endPointer != '\0' ) {
            return false;
        }
        out = parsedValue;
        return true;
    }
};

template<>
struct DefaultParser<float> {
    static bool parse(const char *input, float &out) {
        if ( input == NULL ) {
            return false;
        }
        char *endPointer = NULL;
        const float parsedValue = strtof(input, &endPointer);
        if ( endPointer == input || *endPointer != '\0' ) {
            return false;
        }
        out = parsedValue;
        return true;
    }
};

template<>
struct DefaultParser<bool> {
    static bool parse(const char *input, bool &out) {
        if ( input == NULL ) {
            return false;
        }
        if ( equalsIgnoreCase(input, "true")
             || equalsIgnoreCase(input, "yes")
             || strcmp(input, "1") == 0 ) {
            out = true;
            return true;
        }
        if ( equalsIgnoreCase(input, "false")
             || equalsIgnoreCase(input, "no")
             || strcmp(input, "0") == 0 ) {
            out = false;
            return true;
        }
        return false;
    }

  private:
    static char toLowerAscii(char c) {
        if ( c >= 'A' && c <= 'Z' ) {
            return ((char)(c - 'A' + 'a'));
        }
        return c;
    }

    static bool equalsIgnoreCase(const char *a, const char *b) {
        if ( a == NULL || b == NULL ) {
            return false;
        }
        unsigned long i = 0;
        while ( a[i] != '\0' && b[i] != '\0' ) {
            if ( toLowerAscii(a[i]) != toLowerAscii(b[i]) ) {
                return false;
            }
            i++;
        }
        return a[i] == '\0' && b[i] == '\0';
    }
};

template<>
struct DefaultParser<char *> {
    static bool parse(const char *input, char *&out) {
        if ( input == NULL ) {
            return false;
        }
        out = ((char *)(input));
        return true;
    }
};

template<>
struct DefaultParser<const char *> {
    static bool parse(const char *input, const char *&out) {
        if ( input == NULL ) {
            return false;
        }
        out = input;
        return true;
    }
};

#endif
