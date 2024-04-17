#ifndef __PLANE__
#define __PLANE__

#include "common/linealAlgebra/Vector3D.h"

class Plane {
  public:
    Vector3D normal;
    float d;

    Plane();
};

#endif
