#include <cmath>

#include "common/linealAlgebra/Matrix4x4.h"
#include "common/linealAlgebra/vectorMacros.h"

Matrix4x4 GLOBAL_matrix_identityTransform4x4 = {
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }
};

Matrix4x4
translationMatrix(Vector3D t) {
    Matrix4x4 xf = GLOBAL_matrix_identityTransform4x4;
    xf.m[0][3] = t.x;
    xf.m[1][3] = t.y;
    xf.m[2][3] = t.z;
    return xf;
}

/**
Create scaling, ... transform. The transforms behave identically as the
corresponding transforms in OpenGL
*/
Matrix4x4
rotateMatrix(float angle, Vector3D axis) {
    Matrix4x4 xf = GLOBAL_matrix_identityTransform4x4;
    double x, y, z, c, s, t;

    // Singularity test
    if ((s = VECTORNORM(axis)) < EPSILON ) {
        //Error("Rotate", "Bad rotation axis");
        return xf;
    } else {
        // normalize
        VECTORSCALEINVERSE(s, axis, axis);
    }

    x = axis.x;
    y = axis.y;
    z = axis.z;
    c = std::cos(angle);
    s = std::sin(angle);
    t = 1 - c;
    set3X3Matrix(xf.m,
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
recoverRotationMatrix(Matrix4x4 xf, float *angle, Vector3D *axis) {
    double c, s;

    c = (xf.m[0][0] + xf.m[1][1] + xf.m[2][2] - 1.) * 0.5;
    if ( c > 1. - EPSILON ) {
        *angle = 0.;
        VECTORSET(*axis, 0., 0., 1.);
    } else if ( c < -1. + EPSILON ) {
        *angle = M_PI;
        axis->x = sqrt((xf.m[0][0] + 1.) * 0.5);
        axis->y = sqrt((xf.m[1][1] + 1.) * 0.5);
        axis->z = sqrt((xf.m[2][2] + 1.) * 0.5);

        // Assume x positive, determine sign of y and z */
        if ( xf.m[1][0] < 0. ) {
            axis->y = -axis->y;
        }
        if ( xf.m[2][0] < 0. ) {
            axis->z = -axis->z;
        }
    } else {
        double r;
        *angle = acos(c);
        s = sqrt(1. - c * c);
        r = 1. / (2. * s);
        axis->x = (double) (xf.m[2][1] - xf.m[1][2]) * r;
        axis->y = (double) (xf.m[0][2] - xf.m[2][0]) * r;
        axis->z = (double) (xf.m[1][0] - xf.m[0][1]) * r;
    }
}

/**
xf(p) = xf2(xf1(p))
*/
Matrix4x4
transComposeMatrix(Matrix4x4 xf2, Matrix4x4 xf1) {
    Matrix4x4 xf;

    xf.m[0][0] = xf2.m[0][0] * xf1.m[0][0] + xf2.m[0][1] * xf1.m[1][0] + xf2.m[0][2] * xf1.m[2][0] +
                 xf2.m[0][3] * xf1.m[3][0];
    xf.m[0][1] = xf2.m[0][0] * xf1.m[0][1] + xf2.m[0][1] * xf1.m[1][1] + xf2.m[0][2] * xf1.m[2][1] +
                 xf2.m[0][3] * xf1.m[3][1];
    xf.m[0][2] = xf2.m[0][0] * xf1.m[0][2] + xf2.m[0][1] * xf1.m[1][2] + xf2.m[0][2] * xf1.m[2][2] +
                 xf2.m[0][3] * xf1.m[3][2];
    xf.m[0][3] = xf2.m[0][0] * xf1.m[0][3] + xf2.m[0][1] * xf1.m[1][3] + xf2.m[0][2] * xf1.m[2][3] +
                 xf2.m[0][3] * xf1.m[3][3];

    xf.m[1][0] = xf2.m[1][0] * xf1.m[0][0] + xf2.m[1][1] * xf1.m[1][0] + xf2.m[1][2] * xf1.m[2][0] +
                 xf2.m[1][3] * xf1.m[3][0];
    xf.m[1][1] = xf2.m[1][0] * xf1.m[0][1] + xf2.m[1][1] * xf1.m[1][1] + xf2.m[1][2] * xf1.m[2][1] +
                 xf2.m[1][3] * xf1.m[3][1];
    xf.m[1][2] = xf2.m[1][0] * xf1.m[0][2] + xf2.m[1][1] * xf1.m[1][2] + xf2.m[1][2] * xf1.m[2][2] +
                 xf2.m[1][3] * xf1.m[3][2];
    xf.m[1][3] = xf2.m[1][0] * xf1.m[0][3] + xf2.m[1][1] * xf1.m[1][3] + xf2.m[1][2] * xf1.m[2][3] +
                 xf2.m[1][3] * xf1.m[3][3];

    xf.m[2][0] = xf2.m[2][0] * xf1.m[0][0] + xf2.m[2][1] * xf1.m[1][0] + xf2.m[2][2] * xf1.m[2][0] +
                 xf2.m[2][3] * xf1.m[3][0];
    xf.m[2][1] = xf2.m[2][0] * xf1.m[0][1] + xf2.m[2][1] * xf1.m[1][1] + xf2.m[2][2] * xf1.m[2][1] +
                 xf2.m[2][3] * xf1.m[3][1];
    xf.m[2][2] = xf2.m[2][0] * xf1.m[0][2] + xf2.m[2][1] * xf1.m[1][2] + xf2.m[2][2] * xf1.m[2][2] +
                 xf2.m[2][3] * xf1.m[3][2];
    xf.m[2][3] = xf2.m[2][0] * xf1.m[0][3] + xf2.m[2][1] * xf1.m[1][3] + xf2.m[2][2] * xf1.m[2][3] +
                 xf2.m[2][3] * xf1.m[3][3];

    xf.m[3][0] = xf2.m[3][0] * xf1.m[0][0] + xf2.m[3][1] * xf1.m[1][0] + xf2.m[3][2] * xf1.m[2][0] +
                 xf2.m[3][3] * xf1.m[3][0];
    xf.m[3][1] = xf2.m[3][0] * xf1.m[0][1] + xf2.m[3][1] * xf1.m[1][1] + xf2.m[3][2] * xf1.m[2][1] +
                 xf2.m[3][3] * xf1.m[3][1];
    xf.m[3][2] = xf2.m[3][0] * xf1.m[0][2] + xf2.m[3][1] * xf1.m[1][2] + xf2.m[3][2] * xf1.m[2][2] +
                 xf2.m[3][3] * xf1.m[3][2];
    xf.m[3][3] = xf2.m[3][0] * xf1.m[0][3] + xf2.m[3][1] * xf1.m[1][3] + xf2.m[3][2] * xf1.m[2][3] +
                 xf2.m[3][3] * xf1.m[3][3];

    return xf;
}

/**
This transforms the eye point to the origin and rotates such
that the centre will be on the negative Z axis and the up direction
on the positive Y axis (Y axis positions up, X positions right, Z positions
towards the viewer)
*/
Matrix4x4
lookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up) {
    Matrix4x4 xf = GLOBAL_matrix_identityTransform4x4;
    Vector3D s, X, Y, Z;

    VECTORSUBTRACT(eye, centre, Z);    /* Z positions towards viewer */
    VECTORNORMALIZE(Z);

    VECTORCROSSPRODUCT(up, Z, X);        /* X positions right */
    VECTORNORMALIZE(X);

    VECTORCROSSPRODUCT(Z, X, Y);        /* Y positions up */
    set3X3Matrix(xf.m,            /* view orientation transform */
                 X.x, X.y, X.z,
                 Y.x, Y.y, Y.z,
                 Z.x, Z.y, Z.z);

    VECTORSCALE(-1., eye, s);        /* translate eye to origin */
    return transComposeMatrix(xf, translationMatrix(s));
}

Matrix4x4
perspectiveMatrix(float fov /*radians*/, float aspect, float near, float far) {
    Matrix4x4 xf = GLOBAL_matrix_identityTransform4x4;
    double f = 1. / tan(fov / 2.);

    xf.m[0][0] = f / aspect;
    xf.m[1][1] = f;
    xf.m[2][2] = (near + far) / (near - far);
    xf.m[2][3] = (2 * far * near) / (near - far);
    xf.m[3][2] = -1.;
    xf.m[3][3] = 0.;

    return xf;
}

Matrix4x4
orthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far) {
    Matrix4x4 xf = GLOBAL_matrix_identityTransform4x4;

    xf.m[0][0] = 2.0f / (right - left);
    xf.m[0][3] = -(right + left) / (right - left);

    xf.m[1][1] = 2.0f / (top - bottom);
    xf.m[1][3] = -(top + bottom) / (top - bottom);

    xf.m[2][2] = -2.0f / (far - near);
    xf.m[2][3] = -(far + near) / (far - near);

    return xf;
}
