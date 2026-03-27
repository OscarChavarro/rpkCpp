#ifndef __JAVA_INTEGER__
#define __JAVA_INTEGER__

#include <cstdint>

namespace java {

class Integer {
  public:
    static const int32_t MIN_VALUE = (-2147483647 - 1);
    static const int32_t MAX_VALUE = 2147483647;
};

}

#endif
