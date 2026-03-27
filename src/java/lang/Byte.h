#ifndef __JAVA_BYTE__
#define __JAVA_BYTE__

#include <cstdint>

namespace java {

class Byte {
  public:
    static const int8_t MIN_VALUE = static_cast<int8_t>(-128);
    static const int8_t MAX_VALUE = static_cast<int8_t>(127);
};

}

#endif
