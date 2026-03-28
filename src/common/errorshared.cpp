#include <cstdlib>
#include "java/lang/System.h"
#include "java/util/Formatter.h"
#include <cstdarg>
#include "common/error.h"

/**
Prints an error message. Behaves much like printf. The first argument is the
name of the routine in which the error occurs (optional - can be nullptr)
*/
void logError(const char *routine, const char *text, ...) {
    va_list variableList;

    java::lang::System::err.printf("Error: ");
    if ( routine ) {
        java::lang::System::err.printf("%s(): ", routine);
    }

    va_start(variableList, text);
    const java::lang::String message = java::util::Formatter::vformat(text, variableList);
    va_end(variableList);
    java::lang::System::err.print(message.toCString());

    java::lang::System::err.printf(".\n");
    java::lang::System::err.flush();
}

/**
Fatal error: print message or and exit the program with the specified error code
First argument is a return code. We use negative return codes for
"internal" error messages
*/
__attribute__((noreturn)) void
logFatal(int errcode, const char *routine, const char *text, ...) {
    va_list pvar;

    java::lang::System::err.printf("logFatal error: ");
    if ( routine ) {
        java::lang::System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const java::lang::String message = java::util::Formatter::vformat(text, pvar);
    va_end(pvar);
    java::lang::System::err.print(message.toCString());

    java::lang::System::err.printf(".\n");
    java::lang::System::err.flush();

    exit(errcode);
}

/**
Same, but for warning messages
*/
void
logWarning(const char *routine, const char *text, ...) {
    va_list pvar;

    java::lang::System::err.printf("Warning: ");
    if ( routine ) {
        java::lang::System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const java::lang::String message = java::util::Formatter::vformat(text, pvar);
    va_end(pvar);
    java::lang::System::err.print(message.toCString());

    java::lang::System::err.printf(".\n");
    java::lang::System::err.flush();
}
