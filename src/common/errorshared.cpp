#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include "common/error.h"

static int errorOccurred = false;

/**
stel toestand "geen fouten gebeurd" in
*/
void
logErrorReset() {
    errorOccurred = false;
}

/* geeft false terug indien sinds de vorige oproep zich geen fouten hebben
 * voorgedaan. */
int logErrorOccurred() {
    int errocc = errorOccurred;

    errorOccurred = false;
    return errocc;
}

/* drukt een foutenboodschap af */
void logError(const char *routine, const char *text, ...) {
    va_list pvar;

    fprintf(stderr, "Error: ");
    if ( routine ) {
        fprintf(stderr, "%s(): ", routine);
    }

    va_start(pvar, text);
    vfprintf(stderr, text, pvar);
    va_end(pvar);

    fprintf(stderr, ".\n");
    fflush(stderr);

    errorOccurred = true;
}

/* een fatale fout: druk boodschap of en verlaat het programma
 * met de opgegeven foutencode */
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

    abort();
    exit(errcode);

    errorOccurred = true;
}

void logWarning(const char *routine, const char *text, ...) {
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

