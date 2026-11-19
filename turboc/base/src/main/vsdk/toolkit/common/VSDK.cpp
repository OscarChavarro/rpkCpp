#include <stdio.h>
#include <stdlib.h>

#include "java/lang/String.h"
#include "java/lang/Throwable.h"
#include "vsdk/toolkit/common/VSDK.h"
#ifdef TURBO_C_286_ENABLED
#include "vitralLibC/format.h"
#endif
const double VSDK::EPSILON = 1e-6;

#ifdef __TURBOC__
int snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return written;
}

int vsnprintf(char *buffer, size_t size, const char *format,
    va_list arguments)
{
#ifdef TURBO_C_286_ENABLED
    return crt286VBuffer(buffer, size, format, arguments);
#else
    return vsprintf(buffer, format, arguments);
#endif
}
#endif

#ifdef VSDK_WITHOUT_EXCEPTIONS
VsdkUncaughtThrow vsdkUncaughtThrow;

void VsdkUncaughtThrow::operator<<(const Throwable &exception) const
{
    fprintf(stderr, "Uncaught exception: %s\n", exception.getMessage().c_str());
    abort();
}
#endif

String VSDK::formatDouble(double a)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", a);
    return String(buf);
}

String VSDK::formatDouble(double a, int digits)
{
    if ( digits < 0 ) {
        digits = 0;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", digits, a);
    return String(buf);
}

