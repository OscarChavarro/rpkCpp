#include "java/lang/Exception.h"
#include <cstdio>
#include <cstdlib>

#include "java/lang/String.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
bool Logger::withSystemExit = true;
bool Logger::withFatalExceptions = true;
bool Logger::staticInitialized = false;

/**
C++ port note: Java runs the static initializer on first use of the class,
after the command line properties are set; this is called at the start of
every static method.
*/
void Logger::staticInitialize()
{
    if ( staticInitialized ) {
        return;
    }
    staticInitialized = true;
    // The system property lets a test JVM turn process termination off
    // once for every class (-Dvsdk.withSystemExit=false), instead of each
    // test class toggling it.
    withSystemExit = !java::String("false").equalsIgnoreCase(
        java::System::getProperty("vsdk.withSystemExit", "true"));
    withFatalExceptions = true;
}

bool Logger::isWithSystemExit()
{
    staticInitialize();
    return withSystemExit;
}

void Logger::setWithSystemExit(bool flag) { staticInitialize(); withSystemExit = flag; }
void Logger::setWithFatalExceptions(bool flag) { staticInitialize(); withFatalExceptions = flag; }

void Logger::processFatalError(const java::String& method, const java::String& message, const java::Exception* cause)
{
    staticInitialize();
    if ( withSystemExit ) {
        std::exit(1);
    }
    if ( !withFatalExceptions ) {
        return;
    }
    java::String m = method.isEmpty() ? "VSDK fatal error" : ("VSDK fatal error at " + method);
    if ( !message.isEmpty() ) {
        m += ": " + message;
    }
    if ( cause != nullptr ) {
        m += " | cause: ";
        m += cause->getMessage();
    }
    throw VSDKFatalException(m);
}

void Logger::reportMessage(const java::String& className, int level, const java::String& method, const java::String& message)
{
    fprintf(stderr, "%s\n", message.c_str());
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, nullptr);
    }
}

void Logger::reportMessageWithException(const java::String& className, int level, const java::String& method, const java::String& message, const java::Exception* cause)
{
    if ( cause != nullptr ) {
        fprintf(stderr, "%s | exception: %s\n", message.c_str(), cause->getMessage().c_str());
    } else {
        fprintf(stderr, "%s\n", message.c_str());
    }
    if ( level == FATAL_ERROR ) {
        processFatalError(method, message, cause);
    }
}

