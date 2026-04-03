#include "java/lang/Character.h"

namespace java {

bool
Character::isDigit(int value) {
    return value >= '0' && value <= '9';
}

bool
Character::isSpace(int value) {
    return value == ' ' || (value >= 9 && value <= 13);
}

bool
Character::isLetter(int value) {
    return (value >= 'A' && value <= 'Z')
           || (value >= 'a' && value <= 'z');
}

}
