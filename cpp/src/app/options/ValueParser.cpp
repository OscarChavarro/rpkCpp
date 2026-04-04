#include <cstdlib>
#include <cstring>
#include <climits>
#include <strings.h>

#include "app/options/ValueParser.h"

template<typename T>
bool ValueParser<T>::parse(const char * /*input*/, T & /*out*/) {
    return false;
}

bool ValueParser<int>::parse(const char *input, int &out) {
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

bool ValueParser<float>::parse(const char *input, float &out) {
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

bool ValueParser<bool>::parse(const char *input, bool &out) {
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

bool ValueParser<char *>::parse(const char *input, char *&out) {
    if ( input == nullptr ) {
        return false;
    }
    const unsigned long n = strlen(input) + 1;
    char *copy = new char[n];
    if ( copy == nullptr ) {
        return false;
    }
    memcpy(copy, input, n);
    out = copy;
    return true;
}
