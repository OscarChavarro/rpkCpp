#ifndef __VECTOR_2DD__
#define __VECTOR_2DD__

#include "common/linealAlgebra/Numeric.h"

class Vector2Dd {
  public:
    double u;
    double v;

    Vector2Dd() {
        u = 0.0;
        v = 0.0;
    }
};

inline
void vector2DSet(Vector2Dd &v, double a, double b) {
    v.u = a;
    v.v = b;
}

inline void
vector2DSubtract(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u - q.u;
    r.v = p.v - q.v;
}

inline void
vector2DAdd(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u + q.u;
    r.v = p.v + q.v;
}

inline void
vector2DNegate(Vector2Dd &p) {
    p.u = -p.u;
    p.v = -p.v;
}

inline double
vector2DDeterminant(const Vector2Dd &A, const Vector2Dd &B) {
    return (A.u * B.v - A.v * B.u);
}

#endif
