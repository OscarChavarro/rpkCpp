#ifndef __Transform2x2__
#define __Transform2x2__

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
};

inline void
transformPoint2D(const Matrix2x2 trans, const Vector2D src, Vector2D &dst) {
    Vector2D d;
    d.u = trans.m[0][0] * src.u + trans.m[0][1] * src.v + trans.t[0];
    d.v = trans.m[1][0] * src.u + trans.m[1][1] * src.v + trans.t[1];
    dst = d;
}

inline void
matrix2DPreConcatTransform(const Matrix2x2 xf2, const Matrix2x2 xf1, Matrix2x2 &xf) {
    Matrix2x2 tmpXf{};
    tmpXf.m[0][0] = xf2.m[0][0] * xf1.m[0][0] + xf2.m[0][1] * xf1.m[1][0];
    tmpXf.m[0][1] = xf2.m[0][0] * xf1.m[0][1] + xf2.m[0][1] * xf1.m[1][1];
    tmpXf.m[1][0] = xf2.m[1][0] * xf1.m[0][0] + xf2.m[1][1] * xf1.m[1][0];
    tmpXf.m[1][1] = xf2.m[1][0] * xf1.m[0][1] + xf2.m[1][1] * xf1.m[1][1];
    tmpXf.t[0] = xf2.m[0][0] * xf1.t[0] + xf2.m[0][1] * xf1.t[1] + xf2.t[0];
    tmpXf.t[1] = xf2.m[1][0] * xf1.t[0] + xf2.m[1][1] * xf1.t[1] + xf2.t[1];
    xf = tmpXf;
}

#endif
