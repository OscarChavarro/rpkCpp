#ifndef __PLANE__
#define __PLANE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"

class Plane {
  public:
    Vector3D normal;
    float d;

    Plane();
};

#endif
