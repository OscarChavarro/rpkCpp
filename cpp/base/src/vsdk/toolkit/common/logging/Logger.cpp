#include "vsdk/toolkit/java/lang/String.h"
#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/common/logging/Logger.h"

/**
Prints an error message. Behaves much like printf. The first argument is the
name of the routine in which the error occurs (optional - can be nullptr)
*/
void
Logger::error(const char *routine, const char *text, ...) {
    va_list variableList;

    java::System::err.printf("Error: ");
    if ( routine ) {
        java::System::err.printf("%s(): ", routine);
    }

    va_start(variableList, text);
    const java::String message = java::String::formatCStringToJavaString(text, variableList);
    va_end(variableList);
    java::System::err.print(message.toCString());

    java::System::err.printf(".\n");
    java::System::err.flush();
}

/**
Fatal error: print message or and exit the program with the specified error code
First argument is a return code. We use negative return codes for
"internal" error messages
*/
[[noreturn]] void
Logger::fatal(int errcode, const char *routine, const char *text, ...) {
    va_list pvar;

    java::System::err.printf("logFatal error: ");
    if ( routine ) {
        java::System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const java::String message = java::String::formatCStringToJavaString(text, pvar);
    va_end(pvar);
    java::System::err.print(message.toCString());

    java::System::err.printf(".\n");
    java::System::err.flush();

    java::System::exit(errcode);
}

/**
Same, but for warning messages
*/
void
Logger::warning(const char *routine, const char *text, ...) {
    va_list pvar;

    java::System::err.printf("Warning: ");
    if ( routine ) {
        java::System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const java::String message = java::String::formatCStringToJavaString(text, pvar);
    va_end(pvar);
    java::System::err.print(message.toCString());

    java::System::err.printf(".\n");
    java::System::err.flush();
}
