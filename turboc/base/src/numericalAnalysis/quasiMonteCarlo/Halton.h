#ifndef __HALTON__
#define __HALTON__

#include "common/VSDK.h"

class Halton {
  public:
    static double Halton2(int i);
    static double Halton3(int i);
    static double Halton5(int i);
    static double Halton7(int i);
};

#endif
