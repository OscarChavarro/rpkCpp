#include <cstdio>
#include <cstdarg>

#include "BREP/brep_log.h"

/**
Default callback for showing error messages
*/
static void
brepError(void *client_data, char *message) {
    fprintf(stderr, "error: %s\n", message);
}

static BREP_MSG_CALLBACK_FUNC globalBrepErrorCallbackFunc = brepError;

/**
Prints an error message
*/
void
brepError(void *client_data, const char *routine, const char *text, ...) {
    va_list pvar;
    char buf[BREP_MAX_MESSAGE_LENGTH];

    va_start(pvar, text);
    vsnprintf(buf, BREP_MAX_MESSAGE_LENGTH, text, pvar);
    va_end(pvar);

    globalBrepErrorCallbackFunc(client_data, buf);
}
