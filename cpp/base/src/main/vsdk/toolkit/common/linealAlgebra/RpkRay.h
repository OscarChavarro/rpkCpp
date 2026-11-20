#ifndef RPK_RAY__
#define RPK_RAY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"

class RpkRay {
  public:
    Vector3D position;
    Vector3D direction; // Direction should be normalized
};

#endif
