#ifndef __JAVA_SHORT__
#define __JAVA_SHORT__

#include <cstdint>

namespace java {

class Short {
  public:
    static const int16_t MIN_VALUE = static_cast<int16_t>(-32768);
    static const int16_t MAX_VALUE = static_cast<int16_t>(32767);
};

}

#endif
