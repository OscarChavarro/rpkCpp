#ifndef PLANE__
#define PLANE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

class Plane {
  public:
    Vector3Df normal;
    float d;

    Plane();
};

#endif
