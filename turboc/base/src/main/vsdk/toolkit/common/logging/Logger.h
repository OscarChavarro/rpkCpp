#ifndef __LOGGER__
#define __LOGGER__


#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/lang/Throwable.h"
class Logger {
public:
    enum {
        WARNING = 1,
        ERROR = 2,
        FATAL_ERROR = 3,
        DEBUG = 4,
        VERBOSE = 5
    };

    static void setWithSystemExit(BOOLEAN flag);
    static BOOLEAN isWithSystemExit();
    static void setWithFatalExceptions(BOOLEAN flag);

    static void reportMessage(const String& className, int level, const String& method, const String& message);
    static void reportMessageWithException(const String& className, int level, const String& method, const String& message, const Throwable* cause);

private:
    static BOOLEAN withSystemExit;
    static BOOLEAN withFatalExceptions;
    static void processFatalError(const String& method, const String& message, const Throwable* cause);
};


#endif
