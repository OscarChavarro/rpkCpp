#ifndef __JAVA_ILLEGAL_ARGUMENT_EXCEPTION__
#define __JAVA_ILLEGAL_ARGUMENT_EXCEPTION__

#include "java/lang/RuntimeException.h"

namespace java {

/**
Emulation of `java.lang.IllegalArgumentException`.
*/
class IllegalArgumentException : public RuntimeException {
  public:
    IllegalArgumentException() : RuntimeException() {}
    explicit IllegalArgumentException(const String &message) : RuntimeException(message) {}
    virtual ~IllegalArgumentException() {}
};

}

#endif
