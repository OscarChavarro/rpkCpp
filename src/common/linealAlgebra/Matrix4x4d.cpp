#include "common/linealAlgebra/Matrix4x4d.h"

Matrix4x4d::Matrix4x4d(): m() {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            if ( i == j ) {
                m[i][j] = 1.0;
            } else {
                m[i][j] = 0.0;
            }
        }
    }
}

void
Matrix4x4d::copy(const Matrix4x4d *source) {
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            m[i][j] = source->m[i][j];
        }
    }
}

/**
Transform vector v3b by m4 and put into v3a
*/
void
Matrix4x4d::multiply(Vector3Dd *v3a, const Vector3Dd *v3b) const
{
    Matrix4x4d tmp;

    tmp.m[0][0] = v3b->x * m[0][0] + v3b->y * m[1][0] + v3b->z * m[2][0];
    tmp.m[0][1] = v3b->x * m[0][1] + v3b->y * m[1][1] + v3b->z * m[2][1];
    tmp.m[0][2] = v3b->x * m[0][2] + v3b->y * m[1][2] + v3b->z * m[2][2];

    v3a->x = tmp.m[0][0];
    v3a->y = tmp.m[0][1];
    v3a->z = tmp.m[0][2];
}

/**
Transform p3b by m4 and put into p3a
*/
void
Matrix4x4d::multiplyWithTranslation(Vector3Dd *p3a, const Vector3Dd *p3b) const
{
    multiply(p3a, p3b); // Transform as vector
    p3a->x += m[3][0]; // Translate
    p3a->y += m[3][1];
    p3a->z += m[3][2];
}

void
Matrix4x4d::identity() {
    Matrix4x4d tmp;
    copy(&tmp);
}

/**
Multiply m4b X m4c and put into m4a
*/
void
Matrix4x4d::multiplyMatrix4(Matrix4x4d *m4a, const Matrix4x4d *m4b, const Matrix4x4d *m4c)
{
    Matrix4x4d tmp;
    for ( int i = 3; i >= 0; i-- ) {
        for ( int j = 3; j >= 0; j-- ) {
            tmp.m[i][j] =
                m4b->m[i][0] * m4c->m[0][j] +
                m4b->m[i][1] * m4c->m[1][j] +
                m4b->m[i][2] * m4c->m[2][j] +
                m4b->m[i][3] * m4c->m[3][j];
        }
    }

    m4a->copy(&tmp);
}
