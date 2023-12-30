#ifndef _RAY_H_
#define _RAY_H_

#include "common/linealAlgebra/vectorMacros.h"

class Ray {
  public:
    Vector3D pos;
    Vector3D dir; // direction is supposed to be normalized
};

#endif
