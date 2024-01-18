#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include "BREP/brep_log.h"

/* The default message callback routines ignore the client data */
/* default callback for showing informational messages */
static void brep_info(void *client_data, char *message) {
}

/* default callback for showing error messages */
static void brep_error(void *client_data, char *message) {
    fprintf(stderr, "error: %s\n", message);
}

/* default callback for showing fatal error messages */
static void brep_fatal(void *client_data, char *message) {
    fprintf(stderr, "fatal error: %s\n", message);
    exit(-1);
}

static BREP_MSG_CALLBACK_FUNC brep_info_callback_func = brep_info;
static BREP_MSG_CALLBACK_FUNC brep_error_callback_func = brep_error;
static BREP_MSG_CALLBACK_FUNC brep_fatal_callback_func = brep_fatal;

/**
Prints an informational message
*/
void
BrepInfo(void *client_data, const char *routine, const char *text, ...) {
    va_list pvar;
    char buf[BREP_MAX_MESSAGE_LENGTH];

    va_start(pvar, text);
    vsnprintf(buf, BREP_MAX_MESSAGE_LENGTH, text, pvar);
    va_end(pvar);

    brep_info_callback_func(client_data, buf);
}

/* prints an error message */
void BrepError(void *client_data, const char *routine, const char *text, ...) {
    va_list pvar;
    char buf[BREP_MAX_MESSAGE_LENGTH];

    va_start(pvar, text);
    vsnprintf(buf, BREP_MAX_MESSAGE_LENGTH, text, pvar);
    va_end(pvar);

    brep_error_callback_func(client_data, buf);
}

/* prints a fatal error message */
void BrepFatal(void *client_data, const char *routine, const char *text, ...) {
    va_list pvar;
    char buf[BREP_MAX_MESSAGE_LENGTH];

    va_start(pvar, text);
    vsnprintf(buf, BREP_MAX_MESSAGE_LENGTH, text, pvar);
    va_end(pvar);

    brep_fatal_callback_func(client_data, buf);
}

