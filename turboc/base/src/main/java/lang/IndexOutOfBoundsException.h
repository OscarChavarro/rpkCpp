#ifndef __INDX_OT_OF_BNDS_EXCPTN__
#define __INDX_OT_OF_BNDS_EXCPTN__

#include "java/lang/RuntimeException.h"

/**
Emulation of `java.lang.IndexOutOfBoundsException`.
*/
class IndexOutOfBoundsException : public RuntimeException {
  public:
    IndexOutOfBoundsException() : RuntimeException() {}
    IndexOutOfBoundsException(const String &message) : RuntimeException(message) {}
    virtual ~IndexOutOfBoundsException() {}
};

#endif
