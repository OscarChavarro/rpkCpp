#ifndef __RPK_RAY__
#define __RPK_RAY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

class RpkRay {
  public:
    Vector3Df position;
    Vector3Df direction; // Direction should be normalized
};

#endif
