#ifndef MATRIX_4X4D__
#define MATRIX_4X4D__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Matrix4x4d {
  public:
    double m[4][4];

    Matrix4x4d();

    void multiply(Vector3Dd *v3a, const Vector3Dd *v3b) const;
    void multiplyWithTranslation(Vector3Dd *p3a, const Vector3Dd *p3b) const;

    void copy(const Matrix4x4d *source);
    void identity();

    static void multiplyMatrix4(Matrix4x4d *m4a, const Matrix4x4d *m4b, const Matrix4x4d *m4c);
};

#endif
