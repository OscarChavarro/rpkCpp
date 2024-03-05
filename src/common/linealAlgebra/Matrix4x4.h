#ifndef __MATRIX4X4__
#define __MATRIX4X4__

#include "common/linealAlgebra/Vector4D.h"
#include "common/linealAlgebra/Vector3D.h"

class Matrix4x4 {
  public:
    float m[4][4];
};

inline void
set3X3Matrix(float m[4][4], float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    m[0][0] = a; m[0][1] = b; m[0][2] = c;
    m[1][0] = d; m[1][1] = e; m[1][2] = f;
    m[2][0] = g; m[2][1] = h; m[2][2] = i;
}

inline void
transformPoint3D(const Matrix4x4 trans, const Vector3D src, Vector3D &dst) {
    Vector3D _d_;
    _d_.x = trans.m[0][0] * src.x + trans.m[0][1] * src.y + trans.m[0][2] * src.z;
    _d_.y = trans.m[1][0] * src.x + trans.m[1][1] * src.y + trans.m[1][2] * src.z;
    _d_.z = trans.m[2][0] * src.x + trans.m[2][1] * src.y + trans.m[2][2] * src.z;
    dst = _d_;
}

inline void
transformPoint4D(const Matrix4x4 trans, const Vector4D src, Vector4D &dst) {
    Vector4D d{};
    d.x = trans.m[0][0] * src.x + (trans).m[0][1] * src.y + trans.m[0][2] * src.z + trans.m[0][3] * src.w;
    d.y = trans.m[1][0] * src.x + (trans).m[1][1] * src.y + trans.m[1][2] * src.z + trans.m[1][3] * src.w;
    d.z = trans.m[2][0] * src.x + (trans).m[2][1] * src.y + trans.m[2][2] * src.z + trans.m[2][3] * src.w;
    d.w = trans.m[3][0] * src.x + (trans).m[3][1] * src.y + trans.m[3][2] * src.z + trans.m[3][3] * src.w;
    dst = d;
}

extern Matrix4x4 transComposeMatrix(Matrix4x4 xf2, Matrix4x4 xf1);
extern Matrix4x4 translationMatrix(Vector3D t);
extern Matrix4x4 perspectiveMatrix(float fov /*radians*/, float aspect, float near, float far);
extern Matrix4x4 rotateMatrix(float angle /* radians */, Vector3D axis);
extern Matrix4x4 lookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up);
extern Matrix4x4 orthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far);
extern void recoverRotationMatrix(Matrix4x4 xf, float *angle, Vector3D *axis);

#endif
