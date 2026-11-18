#ifndef __NUMBER_FORMAT_EXCEPTION__
#define __NUMBER_FORMAT_EXCEPTION__

#include "java/lang/IllegalArgumentException.h"

namespace java {

/**
Emulation of `java.lang.NumberFormatException`, thrown by the parse methods
of `Integer` and `Double`.
*/
class NumberFormatException : public IllegalArgumentException {
  public:
    explicit NumberFormatException(const String &message) :
        IllegalArgumentException(message) {}
    virtual ~NumberFormatException() {}
};

}

#endif
