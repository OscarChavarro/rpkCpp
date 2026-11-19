#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Character.h"

BOOLEAN
Character::isDigit(int value) {
    return value >= '0' && value <= '9';
}

BOOLEAN
Character::isSpace(int value) {
    return value == ' ' || (value >= 9 && value <= 13);
}

BOOLEAN
Character::isLetter(int value) {
    return (value >= 'A' && value <= 'Z')
           || (value >= 'a' && value <= 'z');
}

