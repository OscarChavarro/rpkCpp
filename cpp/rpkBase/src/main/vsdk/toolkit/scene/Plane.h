#ifndef __PLANE__
#define __PLANE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Plane {
  public:
    Vector3Dd normal;
    double d;

    Plane();
};

#endif
