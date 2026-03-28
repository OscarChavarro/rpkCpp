#include <cstdlib>

#include "java/lang/System.h"
#include "java/util/Formatter.h"

#include "common/error.h"

namespace {

static java::lang::String
formatToString(const char *format, va_list arguments) {
    if ( format == nullptr ) {
        return java::lang::String();
    }

    char localBuffer[256];
    va_list argumentsCopy;
    va_copy(argumentsCopy, arguments);
    const int required = java::util::Formatter::vformat(localBuffer, static_cast<int>(sizeof(localBuffer)), format, argumentsCopy);
    va_end(argumentsCopy);

    if ( required < 0 ) {
        return java::lang::String();
    }
    if ( required < static_cast<int>(sizeof(localBuffer)) ) {
        return java::lang::String(localBuffer);
    }

    char *dynamicBuffer = new char[required + 1];
    va_copy(argumentsCopy, arguments);
    java::util::Formatter::vformat(dynamicBuffer, required + 1, format, argumentsCopy);
    va_end(argumentsCopy);

    java::lang::String result(dynamicBuffer);
    delete[] dynamicBuffer;
    return result;
}

}

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
    const java::lang::String message = formatToString(text, variableList);
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
    const java::lang::String message = formatToString(text, pvar);
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
    const java::lang::String message = formatToString(text, pvar);
    va_end(pvar);
    java::lang::System::err.print(message.toCString());

    java::lang::System::err.printf(".\n");
    java::lang::System::err.flush();
}
