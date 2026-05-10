#include "java/lang/String.h"
#include "java/lang/System.h"
#include "common/logging/Logger.h"

/**
Prints an error message. Behaves much like printf. The first argument is the
name of the routine in which the error occurs (optional - can be NULL)
*/
void
Logger::error(const char *routine, const char *text, ...) {
    va_list variableList;

    System::err.printf("Error: ");
    if ( routine ) {
        System::err.printf("%s(): ", routine);
    }

    va_start(variableList, text);
    const String message = String::formatCStringToJavaString(text, variableList);
    va_end(variableList);
    System::err.print(message.toCString());

    System::err.printf(".\n");
    System::err.flush();
}

/**
Fatal error: print message or and exit the program with the specified error code
First argument is a return code. We use negative return codes for
"internal" error messages
*/
void
Logger::fatal(int errcode, const char *routine, const char *text, ...) {
    va_list pvar;

    System::err.printf("logFatal error: ");
    if ( routine ) {
        System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const String message = String::formatCStringToJavaString(text, pvar);
    va_end(pvar);
    System::err.print(message.toCString());

    System::err.printf(".\n");
    System::err.flush();

    System::exit(errcode);
}

/**
Same, but for warning messages
*/
void
Logger::warning(const char *routine, const char *text, ...) {
    va_list pvar;

    System::err.printf("Warning: ");
    if ( routine ) {
        System::err.printf("%s(): ", routine);
    }

    va_start(pvar, text);
    const String message = String::formatCStringToJavaString(text, pvar);
    va_end(pvar);
    System::err.print(message.toCString());

    System::err.printf(".\n");
    System::err.flush();
}
