#ifndef RAY__
#define RAY__

#include "common/linealAlgebra/Vector3D.h"

class Ray {
  public:
    Vector3D position;
    Vector3D direction; // Direction should be normalized
};

#endif
