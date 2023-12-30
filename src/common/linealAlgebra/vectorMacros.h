#ifndef _RPK_VECTOR_H_
#define _RPK_VECTOR_H_

#include <cstdio>

#include "common/linealAlgebra/Float.h"
#include "common/linealAlgebra/Vector2D.h"
#include "common/linealAlgebra/Vector2Dd.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector4D.h"

#define XNORMAL 0
#define YNORMAL 1
#define ZNORMAL 2
#define X_GREATER 0x01
#define Y_GREATER 0x02
#define Z_GREATER 0x04
#define XYZ_EQUAL 0x08

#define USE_FUNCTIONS

/**
Fills in x, y, and z component of a vector
*/
#ifdef USE_FUNCTIONS
inline void
VECTORSET(Vector3D &v, const float a, const float b, const float c) {
    v.x = a;
    v.y = b;
    v.z = c;
}

inline void
VECTORSET(Vector3Dd &v, const double a, const double b, const double c) {
    v.x = a;
    v.y = b;
    v.z = c;
}
#else
#define VECTORSET(v, a, b, c) { \
    (v).x = a; \
    (v).y = b; \
    (v).z = c; \
}
#endif

/**
Copies the vector v to d: d = v. They may be different vector types
*/
#ifdef USE_FUNCTIONS
inline void
VECTORCOPY(const Vector3D &v, Vector3D &d) {
    d.x = v.x;
    d.y = v.y;
    d.z = v.z;
}

inline void
VECTORCOPY(const Vector3Dd &v, Vector3Dd &d) {
    d.x = v.x;
    d.y = v.y;
    d.z = v.z;
}

inline void
VECTORCOPY(const Vector3Dd &v, Vector3D &d) {
    d.x = v.x;
    d.y = v.y;
    d.z = v.z;
}

inline void
VECTORCOPY(const Vector3D &v, Vector3Dd &d) {
    d.x = v.x;
    d.y = v.y;
    d.z = v.z;
}
#else
    #define VECTORCOPY(v, d) {(d).x = (v).x; (d).y = (v).y; (d).z = (v).z;}
#endif

/**
Tolerance value for e.g. a vertex position
*/
#ifdef USE_FUNCTIONS
inline float
VECTORTOLERANCE(const Vector3D v) {
    return EPSILON * (fabs(v.x) + fabs(v.y) + fabs(v.z));
}
#else
    #define VECTORTOLERANCE(v) (EPSILON * (fabs((v).x) + fabs((v).y) + fabs((v).z)))
#endif

/**
Two vectors are equal if their components are equal within the given tolerance
*/
#ifdef USE_FUNCTIONS
inline bool
VECTOREQUAL(const Vector3D &v, const Vector3D &w, const float eps) {
    return (
        FLOATEQUAL(v.x, w.x, eps) &&
        FLOATEQUAL(v.y, w.y, eps) &&
        FLOATEQUAL(v.z, w.z, eps)
    );
}
#else
#define VECTOREQUAL(v, w, eps) (       \
    FLOATEQUAL((v).x, (w).x, (eps)) && \
    FLOATEQUAL((v).y, (w).y, (eps)) && \
    FLOATEQUAL((v).z, (w).z, (eps)))
#endif

/**
Vector difference
*/
#ifdef USE_FUNCTIONS
inline void
VECTORSUBTRACT(const Vector3D& a, const Vector3D& b, Vector3D& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

inline void
VECTORSUBTRACT(const Vector3Dd& a, const Vector3Dd& b, Vector3Dd& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

inline void
VECTORSUBTRACT(const Vector3Dd& a, const Vector3Dd& b, Vector3D& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}

inline void
VECTORSUBTRACT(const Vector3D& a, const Vector3D& b, Vector3Dd& d) {
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    d.z = a.z - b.z;
}
#else
#define VECTORSUBTRACT(a, b, d) {\
    (d).x = (a).x - (b).x; \
    (d).y = (a).y - (b).y; \
    (d).z = (a).z - (b).z; \
}
#endif

/**
Vector sum: d = a + b
*/
#ifdef USE_FUNCTIONS
inline void
VECTORADD(const Vector3D &a, const Vector3D &b, Vector3D &d) {
    d.x = a.x + b.x;
    d.y = a.y + b.y;
    d.z = a.z + b.z;
}
#else
#define VECTORADD(a, b, d) { \
    (d).x = (a).x + (b).x; \
    (d).y = (a).y + (b).y; \
    (d).z = (a).z + (b).z; \
}
#endif

/**
Scaled vector sum: d = a + s.b
*/
#ifdef USE_FUNCTIONS
inline void
VECTORSUMSCALED(const Vector3D a, const double s, const Vector3D b, Vector3D &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}

inline void
VECTORSUMSCALED(const Vector3D a, const double s, const Vector3D b, Vector3Dd &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}

inline void
VECTORSUMSCALED(const Vector3Dd a, const double s, const Vector3Dd b, Vector3Dd &d) {
    d.x = a.x + s * b.x;
    d.y = a.y + s * b.y;
    d.z = a.z + s * b.z;
}
#else
    #define VECTORSUMSCALED(a, s, b, d) { \
        (d).x = (a).x + (s) * (b).x; \
        (d).y = (a).y + (s) * (b).y; \
        (d).z = (a).z + (s) * (b).z; \
    }
#endif

/**
Scalar vector product: a.b
*/
#ifdef USE_FUNCTIONS
inline double
VECTORDOTPRODUCT(const Vector3D a, const Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

inline double
VECTORDOTPRODUCT(const Vector3Dd a, const Vector3Dd b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}

inline double
VECTORDOTPRODUCT(const Vector3Dd a, const Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b. z;
}
#else
    #define VECTORDOTPRODUCT(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)
#endif

/**
Square of vector norm: scalar product with itself
*/
#ifdef USE_FUNCTIONS
inline float
VECTORNORM2(const Vector3D &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float
VECTORNORM2(const Vector3Dd &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}
#else
    #define VECTORNORM2(v) ((v).x * (v).x + (v).y * (v).y + (v).z * (v).z)
#endif

/**
Norm of a vector: sqaure root of the sqaure norm
*/
#ifdef USE_FUNCTIONS
inline float
VECTORNORM(const Vector3D &v) {
    return (float)std::sqrt((double)VECTORNORM2(v));
}

inline double
VECTORNORM(const Vector3Dd &v) {
    return (double)std::sqrt((double)VECTORNORM2(v));
}
#else
    #define VECTORNORM(v) (double)sqrt((double)VECTORNORM2(v))
#endif

/**
Scale a vector: d = s.v (s is a real number)
*/
#ifdef USE_FUNCTIONS
inline void
VECTORSCALE(const float s, const Vector3D &v, Vector3D &d) {
    d.x = s * v.x;
    d.y = s * v.y;
    d.z = s * v.z;
}

inline void
VECTORSCALE(const double s, const Vector3Dd &v, Vector3Dd &d) {
    d.x = s * v.x;
    d.y = s * v.y;
    d.z = s * v.z;
}
#else
#define VECTORSCALE(s, v, d) { \
    (d).x = (s) * (v).x; \
    (d).y = (s) * (v).y; \
    (d).z = (s) * (v).z; \
}
#endif

/**
Scales a vector with the inverse of the real number s if not zero: d = (1/s).v
*/
#ifdef USE_FUNCTIONS
inline void
VECTORSCALEINVERSE(const float s, const Vector3D &v, Vector3D &d) {
    float normalizedFactor = s < -EPSILON || s > EPSILON ? 1.0/s : 1.0;
    d.x = normalizedFactor * v.x;
    d.y = normalizedFactor * v.y;
    d.z = normalizedFactor * v.z;
}

inline void
VECTORSCALEINVERSE(const double s, const Vector3Dd &v, Vector3Dd &d) {
    double normalizedFactor = s < -EPSILON || s > EPSILON ? 1.0/s : 1.0;
    d.x = normalizedFactor * v.x;
    d.y = normalizedFactor * v.y;
    d.z = normalizedFactor * v.z;
}

#else
    #define VECTORSCALEINVERSE(s, v, d) { \
        double _is_ = ((s) < -EPSILON || (s) > EPSILON ? 1./(s) : 1.); \
        (d).x = _is_ * (v).x; \
        (d).y = _is_ * (v).y; \
        (d).z = _is_ * (v).z; \
}
#endif

/**
Normalizes a vector: scale it with the inverse of its norm
*/
#ifdef USE_FUNCTIONS
inline void
VECTORNORMALIZE(Vector3D &v) {
    float vectorNorm;
    vectorNorm = VECTORNORM(v);
    VECTORSCALEINVERSE(vectorNorm, v, v);
}

inline void
VECTORNORMALIZE(Vector3Dd &v) {
    double vectorNorm;
    vectorNorm = VECTORNORM(v);
    VECTORSCALEINVERSE(vectorNorm, v, v);
}
#else
    #define VECTORNORMALIZE(v) { \
        double _norm_; _norm_ = VECTORNORM(v); \
        VECTORSCALEINVERSE(_norm_, v, v); \
    }
#endif

/**
In product of two vectors
*/
#ifdef USE_FUNCTIONS
inline void
VECTORCROSSPRODUCT(const Vector3D &a, const Vector3D &b, Vector3D &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    VECTORCOPY(tmp, d);
}

inline void
VECTORCROSSPRODUCT(const Vector3Dd &a, const Vector3Dd &b, Vector3Dd &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    VECTORCOPY(tmp, (d));
}

inline void
VECTORCROSSPRODUCT(const Vector3Dd &a, const Vector3D &b, Vector3D &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    VECTORCOPY(tmp, (d));
}

inline void
VECTORCROSSPRODUCT(const Vector3Dd &a, const Vector3Dd &b, Vector3D &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    VECTORCOPY(tmp, (d));
}

inline void
VECTORCROSSPRODUCT(const Vector3D &a, const Vector3D &b, Vector3Dd &d) {
    Vector3Dd tmp;
    tmp.x = a.y * b.z - a.z * b.y;
    tmp.y = a.z * b.x - a.x * b.z;
    tmp.z = a.x * b.y - a.y * b.x;
    VECTORCOPY(tmp, (d));
}
#else
    #define VECTORCROSSPRODUCT(a, b, d) { \
        Vector3Dd tmp; \
        tmp.x = (a).y * (b).z - (a).z * (b).y; \
        tmp.y = (a).z * (b).x - (a).x * (b).z; \
        tmp.z = (a).x * (b).y - (a).y * (b).x; \
        VECTORCOPY(tmp, (d)); \
    }
#endif

/**
Linear combination of two vectors: d = a.v + b.w
*/
#ifdef USE_FUNCTIONS
inline void
VECTORCOMB2(const float a, const Vector3D &v, const float b, const Vector3D &w, Vector3D &d) {
    d.x = a * v.x + b * w.x;
    d.y = a * v.y + b * w.y;
    d.z = a * v.z + b * w.z;
}
#else
    #define VECTORCOMB2(a, v, b, w, d) { \
        (d).x = (a) * (v).x + (b) * (w).x; \
        (d).y = (a) * (v).y + (b) * (w).y; \
        (d).z = (a) * (v).z + (b) * (w).z; \
    }
#endif

/**
Affine linear combination of two vectors: d = o + a.v + b.w
*/
#ifdef USE_FUNCTIONS
inline void
VECTORCOMB3(const Vector3D &o, const float a, const Vector3D &v, const float b, const Vector3D &w, Vector3D &d) {
    d.x = o.x + a * v.x + b * w.x;
    d.y = o.y + a * v.y + b * w.y;
    d.z = o.z + a * v.z + b * w.z;
}
#else
    #define VECTORCOMB3(o, a, v, b, w, d) { \
        (d).x = (o).x + (a) * (v).x + (b) * (w).x; \
        (d).y = (o).y + (a) * (v).y + (b) * (w).y; \
        (d).z = (o).z + (a) * (v).z + (b) * (w).z; \
    }
#endif

/**
Triple (cross) product: d = (v3 - v2) x (v1 - v2)
*/
#ifdef USE_FUNCTIONS
inline void
VECTORTRIPLECROSSPRODUCT(const Vector3D &v1, const Vector3D &v2, const Vector3D &v3, Vector3D &d) {
    Vector3D _D1, _D2;
    VECTORSUBTRACT(v3, v2, _D1);
    VECTORSUBTRACT(v1, v2, _D2);
    VECTORCROSSPRODUCT(_D1, _D2, d);
}


inline void
VECTORTRIPLECROSSPRODUCT(const Vector3D &v1, const Vector3D &v2, const Vector3D &v3, Vector3Dd &d) {
    Vector3D _D1, _D2;
    VECTORSUBTRACT(v3, v2, _D1);
    VECTORSUBTRACT(v1, v2, _D2);
    VECTORCROSSPRODUCT(_D1, _D2, d);
}

#else
    #define VECTORTRIPLECROSSPRODUCT(v1, v2, v3, d) { \
        Vector3Dd _D1, _D2; \
        VECTORSUBTRACT(v3, v2, _D1); \
        VECTORSUBTRACT(v1, v2, _D2); \
        VECTORCROSSPRODUCT(_D1, _D2, d); \
    }
#endif

/**
Distance between two points in 3D space: s = |p2-p1|
*/
inline float
VECTORDIST(const Vector3D &p1, const Vector3D &p2) {
    Vector3D _D;
    VECTORSUBTRACT(p2, p1, _D);
    return VECTORNORM(_D);
}

/**
Squared distance between two points in 3D space: s = |p2-p1|
*/
inline float
VECTORDIST2(const Vector3D &p1, const Vector3D &p2) {
    Vector3Dd _D;
    VECTORSUBTRACT(p2, p1, _D);
    return VECTORNORM2(_D);
}

/**
Orthogonal component of X with respect to Y. Result is stored in d
*/
#ifdef USE_FUNCTIONS
inline void
VECTORORTHOCOMP(Vector3D &X, Vector3D &Y, Vector3D &d) {
    float dp;
    dp = VECTORDOTPRODUCT(X, Y);
    VECTORSUMSCALED(X, -dp, Y, d);
}

inline void
VECTORORTHOCOMP(Vector3Dd &X, Vector3Dd &Y, Vector3Dd &d) {
    double dp;
    dp = VECTORDOTPRODUCT(X, Y);
    VECTORSUMSCALED(X, -dp, Y, d);
}
#else
    #define VECTORORTHOCOMP(X, Y, d) { \
        double _dotp_; \
        _dotp_ = VECTORDOTPRODUCT(X, Y); \
        VECTORSUMSCALED(X, -_dotp_, Y, d); \
    }
#endif

/**
Given a vector p in 3D space and an index i, which is XNORMAL, YNORMAL
or ZNORMAL, projects the vector on the YZ, XZ or XY plane respectively
*/
#ifdef USE_FUNCTIONS
inline void
VECTORPROJECT(Vector2Dd &r, Vector3D &p, int i) {
    switch(i) {
        case XNORMAL:
            r.u = p.y;
            r.v = p.z;
            break;
        case YNORMAL:
            r.u = p.x;
            r.v = p.z;
            break;
        case ZNORMAL:
            r.u = p.x;
            r.v = p.y;
            break;
    }
}

inline void
VECTORPROJECT(Vector2D &r, Vector3D &p, int i) {
    switch(i) {
        case XNORMAL:
            r.u = p.y;
            r.v = p.z;
            break;
        case YNORMAL:
            r.u = p.x;
            r.v = p.z;
            break;
        case ZNORMAL:
            r.u = p.x;
            r.v = p.y;
            break;
    }
}

#else
    #define VECTORPROJECT(r, p, i) { \
        switch(i) { \
            case XNORMAL: \
                r.u = (p).y; \
                r.v = (p).z; \
                break; \
            case YNORMAL: \
                r.u = (p).x; \
                r.v = (p).z; \
                break; \
            case ZNORMAL: \
                r.u = (p).x; \
                r.v = (p).y; \
                break; \
        } \
    }
#endif

/**
Centre of two points
*/
#ifdef USE_FUNCTIONS
inline void
MIDPOINT(const Vector3D &p1, const Vector3D &p2, Vector3D &m) {
    m.x = 0.5 * (p1.x + p2.x);
    m.y = 0.5 * (p1.y + p2.y);
    m.z = 0.5 * (p1.z + p2.z);
}
#else
    #define MIDPOINT(p1, p2, m) { \
        (m).x = 0.5 * ((p1).x + (p2).x); \
        (m).y = 0.5 * ((p1).y + (p2).y); \
        (m).z = 0.5 * ((p1).z + (p2).z); \
    }
#endif

/**
Point IN Triangle: barycentric parametrisation
*/
#ifdef USE_FUNCTIONS
inline void
PINT(const Vector3D &v0, const Vector3D &v1, const Vector3D &v2, const float u, const float v, Vector3D &p) {
    p.x = v0.x + u * (v1.x - v0.x) + v * (v2.x - v0.x);
    p.y = v0.y + u * (v1.y - v0.y) + v * (v2.y - v0.y);
    p.z = v0.z + u * (v1.z - v0.z) + v * (v2.z - v0.z);
}
#else
    #define PINT(v0, v1, v2, u, v, p) { \
        double _u = (u), _v = (v); \
        (p).x = (v0).x + _u * ((v1).x - (v0).x) + _v * ((v2).x - (v0).x); \
        (p).y = (v0).y + _u * ((v1).y - (v0).y) + _v * ((v2).y - (v0).y); \
        (p).z = (v0).z + _u * ((v1).z - (v0).z) + _v * ((v2).z - (v0).z); \
    }
#endif

/**
Point IN Quadrilateral: bi-linear parametrisation
*/
#ifdef USE_FUNCTIONS
inline void
PINQ(
    const Vector3D &v0,
    const Vector3D &v1,
    const Vector3D &v2,
    const Vector3D &v3,
    const float u,
    const float v,
    Vector3D &p)
{
    double _c = u * v;
    double _b = u - _c;
    double _d = v - _c;
    p.x = v0.x + _b * (v1.x - v0.x) + _c * (v2.x - v0.x)+ _d * (v3.x - v0.x);
    p.y = v0.y + _b * (v1.y - v0.y) + _c * (v2.y - v0.y)+ _d * (v3.y - v0.y);
    p.z = v0.z + _b * (v1.z - v0.z) + _c * (v2.z - v0.z)+ _d * (v3.z - v0.z);
}
#else
    #define PINQ(v0, v1, v2, v3, u, v, p) { \
        double _c=(u)*(v), _b=(u)-_c, _d=(v)-_c; \
        (p).x = (v0).x + (_b) * ((v1).x - (v0).x) + (_c) * ((v2).x - (v0).x)+ (_d) * ((v3).x - (v0).x); \
        (p).y = (v0).y + (_b) * ((v1).y - (v0).y) + (_c) * ((v2).y - (v0).y)+ (_d) * ((v3).y - (v0).y); \
        (p).z = (v0).z + (_b) * ((v1).z - (v0).z) + (_c) * ((v2).z - (v0).z)+ (_d) * ((v3).z - (v0).z); \
    }
#endif

#endif
