#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include "common/error.h"

/**
Prints an error message. Behaves much like printf. The first argument is the
name of the routine in which the error occurs (optional - can be nullptr)
*/
void logError(const char *routine, const char *text, ...) {
    va_list variableList;

    fprintf(stderr, "Error: ");
    if ( routine ) {
        fprintf(stderr, "%s(): ", routine);
    }

    va_start(variableList, text);
    vfprintf(stderr, text, variableList);
    va_end(variableList);

    fprintf(stderr, ".\n");
    fflush(stderr);
}

/**
Fatal error: print message or and exit the program with the specified error code
First argument is a return code. We use negative return codes for
"internal" error messages
*/
void
logFatal(int errcode, const char *routine, const char *text, ...) {
    va_list pvar;

    fprintf(stderr, "logFatal error: ");
    if ( routine ) {
        fprintf(stderr, "%s(): ", routine);
    }

    va_start(pvar, text);
    vfprintf(stderr, text, pvar);
    va_end(pvar);

    fprintf(stderr, ".\n");
    fflush(stderr);

    exit(errcode);
}

/**
Same, but for warning messages
*/
void
logWarning(const char *routine, const char *text, ...) {
    va_list pvar;

    fprintf(stderr, "Warning: ");
    if ( routine ) {
        fprintf(stderr, "%s(): ", routine);
    }

    va_start(pvar, text);
    vfprintf(stderr, text, pvar);
    va_end(pvar);

    fprintf(stderr, ".\n");
    fflush(stderr);
}

