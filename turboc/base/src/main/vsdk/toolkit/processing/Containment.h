#ifndef __CONTAINMENT__
#define __CONTAINMENT__

#include "vsdk/toolkit/common/VSDK.h"

/**
C++98 replacement for a scoped enumeration: values are accessed as
`Containment::VALUE` and objects convert implicitly to the plain `Value` enum,
so they can be compared and used in `switch` statements.
*/
class Containment {
  public:
    enum Value {
        OUTSIDE = -1,
        LIMIT = 0,
        INSIDE = 1
    };

    Containment() : value(OUTSIDE) {}
    Containment(Value inValue) : value(inValue) {}
    explicit Containment(int inValue) : value((Value)inValue) {}
    operator Value() const { return value; }

  private:
    Value value;
};

#endif
