#ifndef __MATRIX4X4__
#define __MATRIX4X4__

#include "common/linealAlgebra/Vector4D.h"
#include "common/linealAlgebra/Vector3D.h"

class Matrix4x4 {
  public:
    float m[4][4];

    void transformPoint3D(Vector3D src, Vector3D &dst) const;
    void transformPoint4D(Vector4D src, Vector4D &dst) const;
    void recoverRotationParameters(float *angle, Vector3D *axis) const;

    void set3X3Matrix(
        float a,
        float b,
        float c,
        float d,
        float e,
        float f,
        float g,
        float h,
        float i);

    static Matrix4x4 createTransComposeMatrix(const Matrix4x4 *xf2, const Matrix4x4 *xf1);
    static Matrix4x4 createTranslationMatrix(Vector3D translation);
    static Matrix4x4 createPerspectiveMatrix(float fieldOfViewInRadians, float aspect, float near, float far);
    static Matrix4x4 createRotationMatrix(float angleInRadians, Vector3D axis);
    static Matrix4x4 createLookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up);
    static Matrix4x4 createOrthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far);
};

inline void
Matrix4x4::set3X3Matrix(
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
Matrix4x4::transformPoint3D(const Vector3D src, Vector3D &dst) const {
    dst.x = m[0][0] * src.x + m[0][1] * src.y + m[0][2] * src.z;
    dst.y = m[1][0] * src.x + m[1][1] * src.y + m[1][2] * src.z;
    dst.z = m[2][0] * src.x + m[2][1] * src.y + m[2][2] * src.z;
}

inline void
Matrix4x4::transformPoint4D(const Vector4D src, Vector4D &dst) const {
    dst.x = m[0][0] * src.x + m[0][1] * src.y + m[0][2] * src.z + m[0][3] * src.w;
    dst.y = m[1][0] * src.x + m[1][1] * src.y + m[1][2] * src.z + m[1][3] * src.w;
    dst.z = m[2][0] * src.x + m[2][1] * src.y + m[2][2] * src.z + m[2][3] * src.w;
    dst.w = m[3][0] * src.x + m[3][1] * src.y + m[3][2] * src.z + m[3][3] * src.w;
}

#endif
