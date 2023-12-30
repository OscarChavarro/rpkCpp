#ifndef _BREP_PRIVATE_H_
#define _BREP_PRIVATE_H_

/* No error message from the library is longer than this */
#define BREP_MAX_MESSAGE_LENGTH 200

/* callback routines for communicating informational, warning, error and
 * fatal error messages. First parameter (if non-null) is a pointer
 * to the client data of the topological entity that caused the
 * error (use and identification of this info is left to the user of
 * this library). The second argument is the message itself. */
typedef void (*BREP_MSG_CALLBACK_FUNC)(void *client_data, char *message);

extern void BrepInfo(void *client_data, const char *routine, const char *text, ...);
extern void BrepError(void *client_data, const char *routine, const char *text, ...);
extern void BrepFatal(void *client_data, const char *routine, const char *text, ...);

#endif
