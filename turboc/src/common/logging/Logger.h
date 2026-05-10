#ifndef __LOGGER__
#define __LOGGER__

#include "common/VSDK.h"

class Logger {
  public:
    static void error(const char *routine, const char *text, ...);
    static void warning(const char *routine, const char *text, ...);
    static void fatal(int errcode, const char *routine, const char *text, ...);
};

#endif
