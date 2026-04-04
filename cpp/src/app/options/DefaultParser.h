#ifndef __DEFAULT_PARSER__
#define __DEFAULT_PARSER__

#include <cstdlib>
#include <cstring>
#include <climits>
#include <strings.h>

template<typename T>
struct DefaultParser {
    static bool parse(const char * /*input*/, T & /*out*/) {
        return false;
    }
};

template<>
struct DefaultParser<int> {
    static bool parse(const char *input, int &out) {
        if ( input == nullptr ) {
            return false;
        }
        char *endPointer = nullptr;
        const long parsedValue = strtol(input, &endPointer, 10);
        if ( endPointer == input || *endPointer != '\0' ) {
            return false;
        }
        if ( parsedValue < static_cast<long>(INT_MIN) || parsedValue > static_cast<long>(INT_MAX) ) {
            return false;
        }
        out = static_cast<int>(parsedValue);
        return true;
    }
};

template<>
struct DefaultParser<long> {
    static bool parse(const char *input, long &out) {
        if ( input == nullptr ) {
            return false;
        }
        char *endPointer = nullptr;
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
        if ( input == nullptr ) {
            return false;
        }
        char *endPointer = nullptr;
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
        if ( input == nullptr ) {
            return false;
        }
        if ( strcasecmp(input, "true") == 0 || strcasecmp(input, "yes") == 0 || strcmp(input, "1") == 0 ) {
            out = true;
            return true;
        }
        if ( strcasecmp(input, "false") == 0 || strcasecmp(input, "no") == 0 || strcmp(input, "0") == 0 ) {
            out = false;
            return true;
        }
        return false;
    }
};

template<>
struct DefaultParser<char *> {
    static bool parse(const char *input, char *&out) {
        if ( input == nullptr ) {
            return false;
        }
        out = const_cast<char *>(input);
        return true;
    }
};

template<>
struct DefaultParser<const char *> {
    static bool parse(const char *input, const char *&out) {
        if ( input == nullptr ) {
            return false;
        }
        out = input;
        return true;
    }
};

#endif
