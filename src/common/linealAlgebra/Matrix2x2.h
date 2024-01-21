#ifndef __Transform2x2__
#define __Transform2x2__

#include <cstdio>

#include "common/linealAlgebra/Vector2D.h"

/*
 * |u'|   |m[0][0]  m[0][1]|   | u |   |t[0]|
 * |  | = |                | * |   | + |    |
 * |v'|   |m[1][0]  m[1][1]|   | v |   |t[1]|
 */
class Matrix2x2 {
  public:
    float m[2][2];
    float t[2];

    void print(FILE *fp) {
        fprintf(fp, "\t%f %f    %f\n", m[0][0], m[0][1], t[0]);
        fprintf(fp, "\t%f %f    %f\n", m[0][1], m[1][1], t[1]);
    }
};

inline void
transformPoint2D(const Matrix2x2 trans, const Vector2D src, Vector2D &dst) {
    Vector2D _d_;
    _d_.u = trans.m[0][0] * src.u + trans.m[0][1] * src.v + trans.t[0];
    _d_.v = trans.m[1][0] * src.u + trans.m[1][1] * src.v + trans.t[1];
    dst = _d_;
}

inline void
PRECONCAT_TRANSFORM2D(const Matrix2x2 xf2, const Matrix2x2 xf1, Matrix2x2 &xf) {
    Matrix2x2 _xf_;
    _xf_.m[0][0] = (xf2).m[0][0] * (xf1).m[0][0] + (xf2).m[0][1] * (xf1).m[1][0];
    _xf_.m[0][1] = (xf2).m[0][0] * (xf1).m[0][1] + (xf2).m[0][1] * (xf1).m[1][1];
    _xf_.m[1][0] = (xf2).m[1][0] * (xf1).m[0][0] + (xf2).m[1][1] * (xf1).m[1][0];
    _xf_.m[1][1] = (xf2).m[1][0] * (xf1).m[0][1] + (xf2).m[1][1] * (xf1).m[1][1];
    _xf_.t[0]  = (xf2).m[0][0] * (xf1).t[0]  + (xf2).m[0][1] * (xf1).t[1] + (xf2).t[0];
    _xf_.t[1]  = (xf2).m[1][0] * (xf1).t[0]  + (xf2).m[1][1] * (xf1).t[1] + (xf2).t[1];
    (xf) = _xf_;
}

#endif
