#ifndef __ILLEGAL_ARGUMENT_EXCEPTION__
#define __ILLEGAL_ARGUMENT_EXCEPTION__

#include "java/lang/RuntimeException.h"

/**
Emulation of `java.lang.IllegalArgumentException`.
*/
class IllegalArgumentException : public RuntimeException {
  public:
    IllegalArgumentException() : RuntimeException() {}
    IllegalArgumentException(const String &message) : RuntimeException(message) {}
    virtual ~IllegalArgumentException() {}
};

#endif
