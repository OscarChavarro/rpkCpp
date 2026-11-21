#include <stdio.h>
#include <stdlib.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
BOOLEAN Logger::withSystemExit = TRUE;
BOOLEAN Logger::withFatalExceptions = TRUE;

void Logger::setWithSystemExit(BOOLEAN flag) { withSystemExit = flag; }
BOOLEAN Logger::isWithSystemExit() { return withSystemExit; }
void Logger::setWithFatalExceptions(BOOLEAN flag) { withFatalExceptions = flag; }

void Logger::processFatalError(const String& method, const String& message, const Throwable* cause)
{
    if ( withSystemExit ) {
        exit(1);
    }
    if ( !withFatalExceptions ) {
        return;
    }
    // Turbo C++ 3.0 does not take String temporaries in conditional
    // expressions
    String m("VSDK fatal error");
    if ( !method.empty() ) {
        m += " at " + method;
    }
    if ( !message.empty() ) {
        m += ": " + message;
    }
    if ( cause != NULL ) {
        m += " | cause: ";
        m += cause->getMessage().c_str();
    }
    throw VSDKFatalException(m);
}

void Logger::reportMessage(const String& /*className*/, int level, const String& method, const String& message)
{
    fprintf(stderr, "%s\n", message.c_str());
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, NULL);
    }
}

void Logger::reportMessageWithException(const String& /*className*/, int level, const String& method, const String& message, const Throwable* cause)
{
    if ( cause != NULL ) {
        fprintf(stderr, "%s | exception: %s\n", message.c_str(), cause->getMessage().c_str());
    } else {
        fprintf(stderr, "%s\n", message.c_str());
    }
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, cause);
    }
}

// Integral constants bound to references (i.e. by `?:`) need a
// definition out of the class in C++98
