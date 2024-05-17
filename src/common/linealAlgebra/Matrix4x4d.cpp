#include "common/linealAlgebra/Matrix4x4d.h"

MATRIX4Dd::MATRIX4Dd(): m() {
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
MATRIX4Dd::copy(const MATRIX4Dd *source) {
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
MATRIX4Dd::multiply(VECTOR3Dd *v3a, const VECTOR3Dd *v3b) const
{
    MATRIX4Dd tmp;

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
MATRIX4Dd::multiplyWithTranslation(VECTOR3Dd *p3a, const VECTOR3Dd *p3b) const
{
    multiply(p3a, p3b); // Transform as vector
    p3a->x += m[3][0]; // Translate
    p3a->y += m[3][1];
    p3a->z += m[3][2];
}

void
MATRIX4Dd::identity() {
    MATRIX4Dd tmp;
    copy(&tmp);
}

/**
Multiply m4b X m4c and put into m4a
*/
void
multiplyMatrix4(MATRIX4Dd *m4a, const MATRIX4Dd *m4b, const MATRIX4Dd *m4c)
{
    MATRIX4Dd tmp;
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
