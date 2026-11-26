#ifndef __JAVA_INDEX_OUT_OF_BOUNDS_EXCEPTION__
#define __JAVA_INDEX_OUT_OF_BOUNDS_EXCEPTION__

#include "java/lang/RuntimeException.h"

namespace java {

/**
Emulation of `java.lang.IndexOutOfBoundsException`.
*/
class IndexOutOfBoundsException : public RuntimeException {
  public:
    IndexOutOfBoundsException() : RuntimeException() {}
    explicit IndexOutOfBoundsException(const String &message) : RuntimeException(message) {}
    virtual ~IndexOutOfBoundsException() {}
};

}

#endif
