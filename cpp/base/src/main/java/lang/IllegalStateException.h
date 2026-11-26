#ifndef __JAVA_ILLEGAL_STATE_EXCEPTION__
#define __JAVA_ILLEGAL_STATE_EXCEPTION__

#include "java/lang/RuntimeException.h"

namespace java {

/**
Emulation of `java.lang.IllegalStateException`.
*/
class IllegalStateException : public RuntimeException {
  public:
    IllegalStateException() : RuntimeException() {}
    explicit IllegalStateException(const String &message) : RuntimeException(message) {}
    virtual ~IllegalStateException() {}
};

}

#endif
