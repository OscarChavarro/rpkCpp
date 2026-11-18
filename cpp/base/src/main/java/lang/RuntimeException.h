#ifndef __JAVA_RUNTIME_EXCEPTION__
#define __JAVA_RUNTIME_EXCEPTION__

#include "java/lang/Exception.h"

namespace java {

/**
Emulation of `java.lang.RuntimeException`.
*/
class RuntimeException : public Exception {
  public:
    RuntimeException() : Exception() {}
    explicit RuntimeException(const String &message) : Exception(message) {}
    virtual ~RuntimeException() {}
};

}

#endif
