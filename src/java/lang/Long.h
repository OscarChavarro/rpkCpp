#ifndef __JAVA_LONG__
#define __JAVA_LONG__

#include <cstdint>

namespace java {

class Long {
  public:
    static const int64_t MIN_VALUE = (-9223372036854775807LL - 1LL);
    static const int64_t MAX_VALUE = 9223372036854775807LL;
};

}

#endif
