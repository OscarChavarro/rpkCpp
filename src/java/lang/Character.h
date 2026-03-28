#ifndef __CHARACTER__
#define __CHARACTER__

#include <cctype>

namespace java {

class Character {
  public:
    static const int MIN_VALUE = 0;
    static const int MAX_VALUE = 65535;
    static bool isDigit(int value);
    static bool isSpace(int value);
    static bool isLetter(int value);
};

inline bool
Character::isDigit(int value) {
    return std::isdigit(static_cast<unsigned char>(value)) != 0;
}

inline bool
Character::isSpace(int value) {
    return std::isspace(static_cast<unsigned char>(value)) != 0;
}

inline bool
Character::isLetter(int value) {
    return std::isalpha(static_cast<unsigned char>(value)) != 0;
}

}

#endif
