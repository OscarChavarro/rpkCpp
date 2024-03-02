#ifndef _RAY_H_
#define _RAY_H_

#include "common/linealAlgebra/Vector3D.h"

class Ray {
  public:
    Vector3D pos;
    Vector3D dir; // direction is supposed to be normalized
};

#endif
