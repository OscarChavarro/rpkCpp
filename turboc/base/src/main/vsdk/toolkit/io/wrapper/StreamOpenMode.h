#ifndef __STREAM_OPEN_MODE__
#define __STREAM_OPEN_MODE__

#include "vsdk/toolkit/common/VSDK.h"

/**
C++98 replacement for a scoped enumeration: values are accessed as
`StreamOpenMode::VALUE` and objects convert implicitly to the plain `Value` enum,
so they can be compared and used in `switch` statements.
*/
class StreamOpenMode {
  public:
    enum Value {
        READ,
        WRITE
    };

    StreamOpenMode() : value(READ) {}
    StreamOpenMode(Value inValue) : value(inValue) {}
    explicit StreamOpenMode(int inValue) : value((Value)inValue) {}
    operator Value() const { return value; }

  private:
    Value value;
};

#endif
