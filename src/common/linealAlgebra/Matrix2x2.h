#ifndef __MATRIX_2x2__
#define __MATRIX_2x2__

#include <cstdio>
#include "common/linealAlgebra/Vector2D.h"

/**
|u'|   |m[0][0]  m[0][1]|   | u |   |t[0]|
|  | = |                | * |   | + |    |
|v'|   |m[1][0]  m[1][1]|   | v |   |t[1]|
*/
class Matrix2x2 {
  public:
    float m[2][2];
    float t[2];

    void print(FILE *fp) const;

    void transformPoint2D(Vector2D src, Vector2D &dst) const;
    void matrix2DPreConcatTransform(Matrix2x2 xf1, Matrix2x2 &xf) const;
};

inline void
Matrix2x2::transformPoint2D(const Vector2D src, Vector2D &dst) const {
    Vector2D d;
    d.u = m[0][0] * src.u + m[0][1] * src.v + t[0];
    d.v = m[1][0] * src.u + m[1][1] * src.v + t[1];
    dst = d;
}

inline void
Matrix2x2::matrix2DPreConcatTransform(const Matrix2x2 xf1, Matrix2x2 &xf) const {
    Matrix2x2 tmpXf{};
    tmpXf.m[0][0] = m[0][0] * xf1.m[0][0] + m[0][1] * xf1.m[1][0];
    tmpXf.m[0][1] = m[0][0] * xf1.m[0][1] + m[0][1] * xf1.m[1][1];
    tmpXf.m[1][0] = m[1][0] * xf1.m[0][0] + m[1][1] * xf1.m[1][0];
    tmpXf.m[1][1] = m[1][0] * xf1.m[0][1] + m[1][1] * xf1.m[1][1];
    tmpXf.t[0] = m[0][0] * xf1.t[0] + m[0][1] * xf1.t[1] + t[0];
    tmpXf.t[1] = m[1][0] * xf1.t[0] + m[1][1] * xf1.t[1] + t[1];
    xf = tmpXf;
}

#endif
