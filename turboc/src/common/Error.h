#ifndef __ERROR__
#define __ERROR__

#include "common/VSDK.h"

class Error {
  public:
    static void error(const char *routine, const char *text, ...);
    static void warning(const char *routine, const char *text, ...);
    static void fatal(int errcode, const char *routine, const char *text, ...);
};

#endif
