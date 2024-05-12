#include <cmath>

#include "java/lang/Math.h"
#include "common/linealAlgebra/Matrix4x4.h"

static Matrix4x4 globalIdentityMatrix = {
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }
};

Matrix4x4
Matrix4x4::createTranslationMatrix(Vector3D translation) {
    Matrix4x4 xf = globalIdentityMatrix;
    xf.m[0][3] = translation.x;
    xf.m[1][3] = translation.y;
    xf.m[2][3] = translation.z;
    return xf;
}

/**
Create scaling, ... transform. The transforms behave identically as the
corresponding transforms in OpenGL
*/
Matrix4x4
Matrix4x4::createRotationMatrix(float angleInRadians, Vector3D axis) {
    Matrix4x4 xf = globalIdentityMatrix;

    // Singularity test
    float s = axis.norm();
    if ( s < EPSILON ) {
        // Bad rotation axis
        return xf;
    } else {
        // Normalize
        axis.inverseScaledCopy(s, axis, EPSILON_FLOAT);
    }

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    float c = java::Math::cos(angleInRadians);
    s = java::Math::sin(angleInRadians);
    float t = 1 - c;
    xf.set3X3Matrix(
        x * x * t + c, x * y * t - z * s, x * z * t + y * s,
        x * y * t + z * s, y * y * t + c, y * z * t - x * s,
        x * z * t - y * s, y * z * t + x * s, z * z * t + c);
    return xf;
}

/**
Recovers the rotation axis and angle from the given rotation matrix.
There is no check whether the transform really is a rotation.
*/
void
Matrix4x4::recoverRotationParameters(float *angle, Vector3D *axis) const {
    float c = (m[0][0] + m[1][1] + m[2][2] - 1.0f) * 0.5f;
    if ( c > 1.0f - EPSILON ) {
        *angle = 0.0f;
        axis->set(0.0f, 0.0f, 1.0f);
    } else if ( c < -1.0f + EPSILON ) {
        *angle = (float)M_PI;
        axis->x = java::Math::sqrt((m[0][0] + 1.0f) * 0.5f);
        axis->y = java::Math::sqrt((m[1][1] + 1.0f) * 0.5f);
        axis->z = java::Math::sqrt((m[2][2] + 1.0f) * 0.5f);

        // Assume x positive, determine sign of y and z
        if ( m[1][0] < 0.0f ) {
            axis->y = -axis->y;
        }
        if ( m[2][0] < 0.0f ) {
            axis->z = -axis->z;
        }
    } else {
        float r;
        *angle = java::Math::acos(c);
        float s = java::Math::sqrt(1.0f - c * c);
        r = 1.0f / (2.0f * s);
        axis->x = (m[2][1] - m[1][2]) * r;
        axis->y = (m[0][2] - m[2][0]) * r;
        axis->z = (m[1][0] - m[0][1]) * r;
    }
}

/**
xf(p) = xf2(xf1(p))
*/
Matrix4x4
Matrix4x4::createTransComposeMatrix(const Matrix4x4 *xf2, const Matrix4x4 *xf1) {
    Matrix4x4 xf{};

    xf.m[0][0] = xf2->m[0][0] * xf1->m[0][0] + xf2->m[0][1] * xf1->m[1][0] + xf2->m[0][2] * xf1->m[2][0] +
                 xf2->m[0][3] * xf1->m[3][0];
    xf.m[0][1] = xf2->m[0][0] * xf1->m[0][1] + xf2->m[0][1] * xf1->m[1][1] + xf2->m[0][2] * xf1->m[2][1] +
                 xf2->m[0][3] * xf1->m[3][1];
    xf.m[0][2] = xf2->m[0][0] * xf1->m[0][2] + xf2->m[0][1] * xf1->m[1][2] + xf2->m[0][2] * xf1->m[2][2] +
                 xf2->m[0][3] * xf1->m[3][2];
    xf.m[0][3] = xf2->m[0][0] * xf1->m[0][3] + xf2->m[0][1] * xf1->m[1][3] + xf2->m[0][2] * xf1->m[2][3] +
                 xf2->m[0][3] * xf1->m[3][3];

    xf.m[1][0] = xf2->m[1][0] * xf1->m[0][0] + xf2->m[1][1] * xf1->m[1][0] + xf2->m[1][2] * xf1->m[2][0] +
                 xf2->m[1][3] * xf1->m[3][0];
    xf.m[1][1] = xf2->m[1][0] * xf1->m[0][1] + xf2->m[1][1] * xf1->m[1][1] + xf2->m[1][2] * xf1->m[2][1] +
                 xf2->m[1][3] * xf1->m[3][1];
    xf.m[1][2] = xf2->m[1][0] * xf1->m[0][2] + xf2->m[1][1] * xf1->m[1][2] + xf2->m[1][2] * xf1->m[2][2] +
                 xf2->m[1][3] * xf1->m[3][2];
    xf.m[1][3] = xf2->m[1][0] * xf1->m[0][3] + xf2->m[1][1] * xf1->m[1][3] + xf2->m[1][2] * xf1->m[2][3] +
                 xf2->m[1][3] * xf1->m[3][3];

    xf.m[2][0] = xf2->m[2][0] * xf1->m[0][0] + xf2->m[2][1] * xf1->m[1][0] + xf2->m[2][2] * xf1->m[2][0] +
                 xf2->m[2][3] * xf1->m[3][0];
    xf.m[2][1] = xf2->m[2][0] * xf1->m[0][1] + xf2->m[2][1] * xf1->m[1][1] + xf2->m[2][2] * xf1->m[2][1] +
                 xf2->m[2][3] * xf1->m[3][1];
    xf.m[2][2] = xf2->m[2][0] * xf1->m[0][2] + xf2->m[2][1] * xf1->m[1][2] + xf2->m[2][2] * xf1->m[2][2] +
                 xf2->m[2][3] * xf1->m[3][2];
    xf.m[2][3] = xf2->m[2][0] * xf1->m[0][3] + xf2->m[2][1] * xf1->m[1][3] + xf2->m[2][2] * xf1->m[2][3] +
                 xf2->m[2][3] * xf1->m[3][3];

    xf.m[3][0] = xf2->m[3][0] * xf1->m[0][0] + xf2->m[3][1] * xf1->m[1][0] + xf2->m[3][2] * xf1->m[2][0] +
                 xf2->m[3][3] * xf1->m[3][0];
    xf.m[3][1] = xf2->m[3][0] * xf1->m[0][1] + xf2->m[3][1] * xf1->m[1][1] + xf2->m[3][2] * xf1->m[2][1] +
                 xf2->m[3][3] * xf1->m[3][1];
    xf.m[3][2] = xf2->m[3][0] * xf1->m[0][2] + xf2->m[3][1] * xf1->m[1][2] + xf2->m[3][2] * xf1->m[2][2] +
                 xf2->m[3][3] * xf1->m[3][2];
    xf.m[3][3] = xf2->m[3][0] * xf1->m[0][3] + xf2->m[3][1] * xf1->m[1][3] + xf2->m[3][2] * xf1->m[2][3] +
                 xf2->m[3][3] * xf1->m[3][3];

    return xf;
}

/**
This transforms the eye point to the origin and rotates such
that the centre will be on the negative Z axis and the up direction
on the positive Y axis (Y axis positions up, X positions right, Z positions
towards the viewer)
*/
Matrix4x4
Matrix4x4::createLookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up) {
    Matrix4x4 xf = globalIdentityMatrix;
    Vector3D s;
    Vector3D X;
    Vector3D Y;
    Vector3D Z;

    Z.subtraction(eye, centre); // Z positions towards viewer
    Z.normalize(EPSILON_FLOAT);

    X.crossProduct(up, Z); // X positions right
    X.normalize(EPSILON_FLOAT);

    Y.crossProduct(Z, X); // Y positions up
    xf.set3X3Matrix(
        X.x, X.y, X.z, // View orientation transform
        Y.x, Y.y, Y.z,
        Z.x, Z.y, Z.z);

    s.scaledCopy(-1.0, eye); // Translate eye to origin
    Matrix4x4 t = Matrix4x4::createTranslationMatrix(s);
    return Matrix4x4::createTransComposeMatrix(&xf, &t);
}

Matrix4x4
Matrix4x4::createPerspectiveMatrix(float fieldOfViewInRadians, float aspect, float near, float far) {
    Matrix4x4 xf = globalIdentityMatrix;
    float f = 1.0f / java::Math::tan(fieldOfViewInRadians / 2.0f);

    xf.m[0][0] = f / aspect;
    xf.m[1][1] = f;
    xf.m[2][2] = (near + far) / (near - far);
    xf.m[2][3] = (2 * far * near) / (near - far);
    xf.m[3][2] = -1.0f;
    xf.m[3][3] = 0.0f;

    return xf;
}

Matrix4x4
Matrix4x4::createOrthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far) {
    Matrix4x4 xf = globalIdentityMatrix;

    xf.m[0][0] = 2.0f / (right - left);
    xf.m[0][3] = -(right + left) / (right - left);

    xf.m[1][1] = 2.0f / (top - bottom);
    xf.m[1][3] = -(top + bottom) / (top - bottom);

    xf.m[2][2] = -2.0f / (far - near);
    xf.m[2][3] = -(far + near) / (far - near);

    return xf;
}
