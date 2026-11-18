#ifndef __LOGGER__
#define __LOGGER__


#include "java/lang/Exception.h"
#include "java/lang/String.h"
class Logger {
public:
    static const int WARNING = 1;
    static const int ERROR = 2;
    static const int FATAL_ERROR = 3;
    static const int DEBUG = 4;
    static const int VERBOSE = 5;

    static bool isWithSystemExit();
    static void setWithSystemExit(bool flag);
    static void setWithFatalExceptions(bool flag);

    static void reportMessage(const java::String& className, int level, const java::String& method, const java::String& message);
    static void reportMessageWithException(const java::String& className, int level, const java::String& method, const java::String& message, const java::Exception* cause);

private:
    static bool withSystemExit;
    static bool withFatalExceptions;
    /// C++ port note: emulates the lazy Java static initializer.
    static bool staticInitialized;
    static void staticInitialize();
    static void processFatalError(const java::String& method, const java::String& message, const java::Exception* cause);
};


#endif
