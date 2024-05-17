#ifndef __MATRIX_4X4D__
#define __MATRIX_4X4D__

#include "common/linealAlgebra/Vector3Dd.h"

class MATRIX4Dd {
  public:
    double m[4][4];

    MATRIX4Dd();

    void multiply(VECTOR3Dd *v3a, const VECTOR3Dd *v3b) const;
    void multiplyWithTranslation(VECTOR3Dd *p3a, const VECTOR3Dd *p3b) const;

    void copy(const MATRIX4Dd *source);
    void identity();
};

extern void multiplyMatrix4(MATRIX4Dd *m4a, const MATRIX4Dd *m4b, const MATRIX4Dd *m4c);

#endif
