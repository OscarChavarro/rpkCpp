#ifndef __MGF_TRANSFORM__
#define __MGF_TRANSFORM__

#include "common/linealAlgebra/Matrix4x4d.h"

class MgfTransform {
  public:
    MATRIX4Dd transformMatrix;
    double scaleFactor;

    MgfTransform();
};

#endif
