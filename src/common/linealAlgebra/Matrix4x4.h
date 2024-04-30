#ifndef __MATRIX4X4__
#define __MATRIX4X4__

#include "common/linealAlgebra/Vector4D.h"
#include "common/linealAlgebra/Vector3D.h"

class Matrix4x4 {
  public:
    float m[4][4];
};

inline void
set3X3Matrix(
    float m[4][4],
    const float a,
    const float b,
    const float c,
    const float d,
    const float e,
    const float f,
    const float g,
    const float h,
    const float i)
{
    m[0][0] = a; m[0][1] = b; m[0][2] = c;
    m[1][0] = d; m[1][1] = e; m[1][2] = f;
    m[2][0] = g; m[2][1] = h; m[2][2] = i;
}

inline void
transformPoint3D(const Matrix4x4 *trans, const Vector3D src, Vector3D &dst) {
    dst.x = trans->m[0][0] * src.x + trans->m[0][1] * src.y + trans->m[0][2] * src.z;
    dst.y = trans->m[1][0] * src.x + trans->m[1][1] * src.y + trans->m[1][2] * src.z;
    dst.z = trans->m[2][0] * src.x + trans->m[2][1] * src.y + trans->m[2][2] * src.z;
}

inline void
transformPoint4D(const Matrix4x4 *trans, const Vector4D src, Vector4D &dst) {
    dst.x = trans->m[0][0] * src.x + trans->m[0][1] * src.y + trans->m[0][2] * src.z + trans->m[0][3] * src.w;
    dst.y = trans->m[1][0] * src.x + trans->m[1][1] * src.y + trans->m[1][2] * src.z + trans->m[1][3] * src.w;
    dst.z = trans->m[2][0] * src.x + trans->m[2][1] * src.y + trans->m[2][2] * src.z + trans->m[2][3] * src.w;
    dst.w = trans->m[3][0] * src.x + trans->m[3][1] * src.y + trans->m[3][2] * src.z + trans->m[3][3] * src.w;
}

extern Matrix4x4 transComposeMatrix(const Matrix4x4 *xf2, const Matrix4x4 *xf1);
extern Matrix4x4 translationMatrix(Vector3D translation);
extern Matrix4x4 perspectiveMatrix(float fieldOfViewInRadians /*radians*/, float aspect, float near, float far);
extern Matrix4x4 createRotationMatrix(float angle /* radians */, Vector3D axis);
extern Matrix4x4 lookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up);
extern Matrix4x4 orthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far);
extern void recoverRotationMatrix(const Matrix4x4 *xf, float *angle, Vector3D *axis);

#endif
