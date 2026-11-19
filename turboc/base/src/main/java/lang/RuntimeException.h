#ifndef __RUNTIME_EXCEPTION__
#define __RUNTIME_EXCEPTION__

#include "java/lang/Exception.h"

/**
Emulation of `java.lang.RuntimeException`.
*/
class RuntimeException : public Exception {
  public:
    RuntimeException() : Exception() {}
    RuntimeException(const String &message) : Exception(message) {}
    virtual ~RuntimeException() {}
};

#endif
