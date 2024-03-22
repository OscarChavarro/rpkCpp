/**
Routines for 3-d vectors
*/

#include <cstring>

#include "common/mymath.h"
#include "io/mgf/Vector3Dd.h"

MATRIX4Dd GLOBAL_mgf_m4Ident = MAT4IDENT;

static MATRIX4Dd globalM4Tmp; // For efficiency

/**
Normalize a vector, return old magnitude
*/
double
normalize(double *v)
{
    static double len;

    len = dotProduct(v, v);

    if ( len <= 0.0 ) {
        return 0.0;
    }

    if ( len <= 1.0 + EPSILON && len >= 1.0 - EPSILON) {
        // First order approximation
        len = 0.5 + 0.5 * len;
    } else {
        len = std::sqrt(len);
    }

    v[0] /= len;
    v[1] /= len;
    v[2] /= len;

    return len;
}

/**
Cross product of two vectors
result = a X b
*/
void
floatCrossProduct(VECTOR3Dd result, const VECTOR3Dd a, const VECTOR3Dd b)
{
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

/**
Compute u and v given w (normalized)
*/
void
mgfMakeAxes(double *u, double *v, const double *w)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;

    int i;
    for ( i = 0; i < 3; i++ ) {
        if ( w[i] > -0.6 && w[i] < 0.6 ) {
            break;
        }
    }
    v[i] = 1.0;

    floatCrossProduct(u, v, w);
    normalize(u);
    floatCrossProduct(v, w, u);
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
multiplyP3(double *p3a, double *p3b, double (*m4)[4])
{
    multiplyV3(p3a, p3b, m4); // Transform as vector
    p3a[0] += m4[3][0]; // Translate
    p3a[1] += m4[3][1];
    p3a[2] += m4[3][2];
}

void
copyMat4(double (*m4a)[4], MATRIX4Dd m4b) {
    memcpy((char *)m4a,(char *)m4b,sizeof(MATRIX4Dd));
}

void
setIdent4(double (*m4a)[4]) {
    copyMat4(m4a, GLOBAL_mgf_m4Ident);
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

/**
Returns squared distance between the two vectors
*/
double
distanceSquared(VECTOR3Dd *v1, VECTOR3Dd *v2) {
    VECTOR3Dd d;

    d[0] = (*v2)[0] - (*v1)[0];
    d[1] = (*v2)[1] - (*v1)[1];
    d[2] = (*v2)[2] - (*v1)[2];
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}
