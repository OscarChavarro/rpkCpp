#ifndef __CHARACTER__
#define __CHARACTER__

namespace java {

class Character {
  public:
    static const int MIN_VALUE = 0;
    static const int MAX_VALUE = 65535;
    static char min(char a, char b);
};

inline char
Character::min(char a, char b) {
    return a < b ? a : b;
}

}

#endif
