#ifndef __TRANSFORM_CONTEXT_DATA__
#define __TRANSFORM_CONTEXT_DATA__

#include "common/linealAlgebra/Matrix4x4d.h"

class TransformContext {
  public:
    Matrix4x4d transformMatrix;
    double scaleFactor;

    TransformContext();
};

#endif
