#ifndef __Transform2x2__
#define __Transform2x2__

/*
 * |u'|   |m[0][0]  m[0][1]|   | u |   |t[0]|
 * |  | = |                | * |   | + |    |
 * |v'|   |m[1][0]  m[1][1]|   | v |   |t[1]|
 */
class Matrix2x2 {
  public:
    float m[2][2];
    float t[2];
};

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
