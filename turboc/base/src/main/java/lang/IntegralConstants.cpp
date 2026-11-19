#include <limits.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Byte.h"
#include "java/lang/Integer.h"
#include "java/lang/Long.h"
#include "java/lang/Short.h"
#include "java/util/Random.h"

// The integral constants of the classes are defined here, out of them:
// Turbo C++ 3.0 accepts no initializers in a class. Integer is the C++ int
// (16 bits on Turbo C++), and Long and Random need a 64 bit long long
// (Turbo C++ takes long long as a 32 bit long)
const char Byte::MIN_VALUE = ((unsigned char)(-128));
const char Byte::MAX_VALUE = ((unsigned char)(127));
const int Integer::MIN_VALUE = INT_MIN;
const int Integer::MAX_VALUE = INT_MAX;
#ifdef VSDK_HAS_LONG_LONG
const long long Long::MIN_VALUE = (-VSDK_LL(9223372036854775807) - VSDK_LL(1));
const long long Long::MAX_VALUE = VSDK_LL(9223372036854775807);
#else
const long long Long::MIN_VALUE = LONG_MIN;
const long long Long::MAX_VALUE = LONG_MAX;
#endif
const short Short::MIN_VALUE = ((short)(-32768));
const short Short::MAX_VALUE = ((short)(32767));
const unsigned long long Random::MULTIPLIER = VSDK_ULL(0x5DEECE66D);
const unsigned long long Random::ADDEND = VSDK_ULL(0xB);
const unsigned long long Random::MASK = (VSDK_ULL(1) << 48) - 1;
