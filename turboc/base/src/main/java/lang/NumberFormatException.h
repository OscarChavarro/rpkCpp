#ifndef __NUMBER_FORMAT_EXCEPTION__
#define __NUMBER_FORMAT_EXCEPTION__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/IllegalArgumentException.h"

/**
Emulation of `java.lang.NumberFormatException`, thrown by the parse methods
of `Integer` and `Double`.
*/
class NumberFormatException : public IllegalArgumentException {
public:
    explicit NumberFormatException(const String& message)
        : IllegalArgumentException(message) {}
    virtual ~NumberFormatException() {}
};

#endif
