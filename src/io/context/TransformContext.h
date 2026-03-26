#ifndef __TRANSFORM_CONTEXT_DATA__
#define __TRANSFORM_CONTEXT_DATA__

#include "common/linealAlgebra/Matrix4x4d.h"

class TransformContext {
  public:
    MATRIX4Dd transformMatrix;
    double scaleFactor;

    TransformContext();
};

#endif
