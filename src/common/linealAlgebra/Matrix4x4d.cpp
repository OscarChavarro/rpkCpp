#include <cstring>

#include "common/linealAlgebra/Matrix4x4d.h"

#define MATRIX_4X4_IDENTITY { {1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0} }

MATRIX4Dd globalMatrix4Ident = MATRIX_4X4_IDENTITY;

static MATRIX4Dd globalM4Tmp; // For efficiency

void
copyMat4(double (*m4a)[4], MATRIX4Dd m4b) {
    memcpy((char *)m4a,(char *)m4b,sizeof(MATRIX4Dd));
}

/**
Transform vector v3b by m4 and put into v3a
*/
void
multiplyV3(double *v3a, const double *v3b, double (*m4)[4])
{
    globalM4Tmp[0][0] = v3b[0] * m4[0][0] + v3b[1] * m4[1][0] + v3b[2] * m4[2][0];
    globalM4Tmp[0][1] = v3b[0] * m4[0][1] + v3b[1] * m4[1][1] + v3b[2] * m4[2][1];
    globalM4Tmp[0][2] = v3b[0] * m4[0][2] + v3b[1] * m4[1][2] + v3b[2] * m4[2][2];

    v3a[0] = globalM4Tmp[0][0];
    v3a[1] = globalM4Tmp[0][1];
    v3a[2] = globalM4Tmp[0][2];
}

/**
Transform p3b by m4 and put into p3a
*/
void
multiplyP3(double *p3a, const double *p3b, double (*m4)[4])
{
    multiplyV3(p3a, p3b, m4); // Transform as vector
    p3a[0] += m4[3][0]; // Translate
    p3a[1] += m4[3][1];
    p3a[2] += m4[3][2];
}

void
setIdent4(double (*m4a)[4]) {
    copyMat4(m4a, globalMatrix4Ident);
}

/**
Multiply m4b X m4c and put into m4a
*/
void
multiplyMatrix4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4])
{
    for ( int i = 4; i--; ) {
        for ( int j = 4; j--; ) {
            globalM4Tmp[i][j] = m4b[i][0] * m4c[0][j] +
                                m4b[i][1] * m4c[1][j] +
                                m4b[i][2] * m4c[2][j] +
                                m4b[i][3] * m4c[3][j];
        }
    }

    copyMat4(m4a, globalM4Tmp);
}
