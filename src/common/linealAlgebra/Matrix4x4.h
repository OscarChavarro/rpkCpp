#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "common/linealAlgebra/Vector4D.h"
#include "common/linealAlgebra/Vector3D.h"

class Matrix4x4 {
  public:
    float m[4][4];
};

#define TRANSFORM_POINT_2D(trans, src, dst) { \
    Vector2D _d_; \
    _d_.u = (trans).m[0][0] * (src).u + (trans).m[0][1] * (src).v + (trans).t[0]; \
    _d_.v = (trans).m[1][0] * (src).u + (trans).m[1][1] * (src).v + (trans).t[1]; \
    (dst) = _d_; \
}

#define PRINT_TRANSFORM2D(fp, trans) { \
    fprintf(fp, "\t%f %f    %f\n", (trans).m[0][0], (trans).m[0][1], (trans).t[0]); \
    fprintf(fp, "\t%f %f    %f\n", (trans).m[0][1], (trans).m[1][1], (trans).t[1]); \
}

#define SET_3X3MATRIX(m, a, b, c, d, e, f, g, h, i) { \
    m[0][0] = a; m[0][1] = b; m[0][2] = c; \
    m[1][0] = d; m[1][1] = e; m[1][2] = f; \
    m[2][0] = g; m[2][1] = h; m[2][2] = i; \
}

#define TRANSFORM_POINT_3D(trans, src, dst) { \
    Vector3D _d_; \
    _d_.x = (trans).m[0][0] * (src).x + (trans).m[0][1] * (src).y + (trans).m[0][2] * (src).z + (trans).m[0][3]; \
    _d_.y = (trans).m[1][0] * (src).x + (trans).m[1][1] * (src).y + (trans).m[1][2] * (src).z + (trans).m[1][3]; \
    _d_.z = (trans).m[2][0] * (src).x + (trans).m[2][1] * (src).y + (trans).m[2][2] * (src).z + (trans).m[2][3]; \
    (dst) = _d_; \
}

#define TRANSFORM_VECTOR_3D(trans, src, dst) { \
    Vector3D _d_; \
    _d_.x = (trans).m[0][0] * (src).x + (trans).m[0][1] * (src).y + (trans).m[0][2] * (src).z; \
    _d_.y = (trans).m[1][0] * (src).x + (trans).m[1][1] * (src).y + (trans).m[1][2] * (src).z; \
    _d_.z = (trans).m[2][0] * (src).x + (trans).m[2][1] * (src).y + (trans).m[2][2] * (src).z; \
    (dst) = _d_; \
}

#define TRANSFORM_POINT_4D(trans, src, dst) { \
    Vector4D _d_; \
    _d_.x = (trans).m[0][0] * (src).x + (trans).m[0][1] * (src).y + (trans).m[0][2] * (src).z + (trans).m[0][3] * (src).w; \
    _d_.y = (trans).m[1][0] * (src).x + (trans).m[1][1] * (src).y + (trans).m[1][2] * (src).z + (trans).m[1][3] * (src).w; \
    _d_.z = (trans).m[2][0] * (src).x + (trans).m[2][1] * (src).y + (trans).m[2][2] * (src).z + (trans).m[2][3] * (src).w; \
    _d_.w = (trans).m[3][0] * (src).x + (trans).m[3][1] * (src).y + (trans).m[3][2] * (src).z + (trans).m[3][3] * (src).w; \
    (dst) = _d_; \
}

extern Matrix4x4 IdentityTransform4x4;

/* xf(p) = xf2(xf1(p)) */
extern Matrix4x4 TransCompose(Matrix4x4 xf2, Matrix4x4 xf1);

extern Matrix4x4 Translate(Vector3D t);

extern Matrix4x4 Perspective(float fov /*radians*/, float aspect, float near, float far);

/* Create scaling, ... transform. The transforms behave identically as the
 * corresponding transforms in OpenGL. */
extern Matrix4x4 Rotate(float angle /* radians */, Vector3D axis);

extern Matrix4x4 LookAt(Vector3D eye, Vector3D centre, Vector3D up);

extern Matrix4x4 Ortho(float left, float right, float bottom, float top, float near, float far);

/* Recovers the rotation axis and angle from the given rotation matrix.
 * There is no check whether the transform really is a rotation. */
extern void RecoverRotation(Matrix4x4 xf, float *angle, Vector3D *axis);

#endif
