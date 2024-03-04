#ifndef __ERROR__
#define __ERROR__

#define ISNAN(d) (false)

#define PNAN(d)

extern void logError(const char *routine, const char *text, ...);
extern void logWarning(const char *routine, const char *text, ...);
extern void logFatal(int errcode, const char *routine, const char *text, ...);

#endif
