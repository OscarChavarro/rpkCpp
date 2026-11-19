#ifndef __EXCEPTION__
#define __EXCEPTION__

#include "java/lang/Throwable.h"

/**
Emulation of `java.lang.Exception`.
*/
class Exception : public Throwable {
  public:
    Exception() : Throwable() {}
    Exception(const String &message) : Throwable(message) {}
    virtual ~Exception() {}
};

#endif
