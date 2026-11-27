#ifndef __TRANSFORM_CONTEXT__
#define __TRANSFORM_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

class TransformContext {
  public:
    Matrix4x4d transformMatrix;
    double scaleFactor;

    TransformContext();
};

#endif
