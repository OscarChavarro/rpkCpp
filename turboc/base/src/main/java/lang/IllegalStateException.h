#ifndef __ILLEGAL_STATE_EXCEPTION__
#define __ILLEGAL_STATE_EXCEPTION__

#include "java/lang/RuntimeException.h"

/**
Emulation of `java.lang.IllegalStateException`.
*/
class IllegalStateException : public RuntimeException {
  public:
    IllegalStateException() : RuntimeException() {}
    IllegalStateException(const String &message) : RuntimeException(message) {}
    virtual ~IllegalStateException() {}
};

#endif
