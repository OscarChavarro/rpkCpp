/* error.h: for printing warning/error/fatal error messages */

#ifndef _RPK_ERROR_H_
#define _RPK_ERROR_H_

#define ISNAN(d) (false)

#define PNAN(d)

/* prints an error message. Behaves much like printf. The first argument is the
 * name of the routine in which the error occurs (optional - can be nullptr) */
extern void Error(const char *routine, const char *text, ...);

/* same, but for warning messages */
extern void Warning(const char *routine, const char *text, ...);

/* same, but for fatal error messages (also aborts the program).
 * First argument is a return code. We use negative return codes for
 * "internal" error messages. */
extern void Fatal(int errcode, const char *routine, const char *text, ...);

/* returns false if no errors have been reported since the last call to this
 * routine. */
extern int ErrorOccurred();

/* set state to "no errors occured" */
extern void ErrorReset();

#endif
