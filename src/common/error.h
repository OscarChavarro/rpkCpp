#ifndef __ERROR__
#define __ERROR__

extern void logError(const char *routine, const char *text, ...);
extern void logWarning(const char *routine, const char *text, ...);
extern void logFatal(int errcode, const char *routine, const char *text, ...);

#endif
