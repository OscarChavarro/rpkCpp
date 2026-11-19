#ifndef __LOGGER_FORMAT__
#define __LOGGER_FORMAT__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

/**
Turboc port note: the VITRAL turboc `String` has no variadic `format`, so this
rpk-only helper stands in for the `java::String::format(...)` calls that the
cpp port passes to `Logger::reportMessage`.
*/
class LoggerFormat {
  public:
    static String format(const char *text, ...);
};

#endif
