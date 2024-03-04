#ifndef __RAY__
#define __RAY__

#include "common/linealAlgebra/Vector3D.h"

class Ray {
  public:
    Vector3D pos;
    Vector3D dir; // Direction is supposed to be normalized
};

#endif
