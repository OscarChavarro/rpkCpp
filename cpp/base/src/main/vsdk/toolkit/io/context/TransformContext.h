#ifndef TRANSFORM_CONTEXT_DATA__
#define TRANSFORM_CONTEXT_DATA__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

class TransformContext {
  public:
    Matrix4x4d transformMatrix;
    double scaleFactor;

    TransformContext();
};

#endif
