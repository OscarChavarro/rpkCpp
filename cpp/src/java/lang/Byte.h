#ifndef __JAVA_BYTE__
#define __JAVA_BYTE__

namespace java {

class Byte {
  public:
    static const char MIN_VALUE = static_cast<unsigned char>(-128);
    static const char MAX_VALUE = static_cast<unsigned char>(127);
};

}

#endif
