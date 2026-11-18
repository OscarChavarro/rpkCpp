#ifndef __JAVA_EXCEPTION__
#define __JAVA_EXCEPTION__

#include "java/lang/Throwable.h"

namespace java {

/**
Emulation of `java.lang.Exception`.
*/
class Exception : public Throwable {
  public:
    Exception() : Throwable() {}
    explicit Exception(const String &message) : Throwable(message) {}
    virtual ~Exception() {}
};

}

#endif
