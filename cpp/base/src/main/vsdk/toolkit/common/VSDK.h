#ifndef __VSDK__
#define __VSDK__


#include "java/lang/String.h"
class VSDK {
public:
    static const double EPSILON;

    static java::String formatDouble(double a);
    static java::String formatDouble(double a, int digits);
};


#endif
