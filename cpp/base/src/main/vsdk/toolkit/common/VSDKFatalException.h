#ifndef __VSDK_FATAL_EXCEPTION__
#define __VSDK_FATAL_EXCEPTION__

#include "java/lang/RuntimeException.h"
#include "java/lang/String.h"

class VSDKFatalException : public java::RuntimeException {
public:
    explicit VSDKFatalException(const java::String& message) : java::RuntimeException(message) {}
};

#endif
