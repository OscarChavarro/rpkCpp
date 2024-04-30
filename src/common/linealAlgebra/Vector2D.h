#ifndef __VECTOR_2D__
#define __VECTOR_2D__

class Vector2D {
  public:
    float u;
    float v;

    Vector2D() {
        u = 0.0;
        v = 0.0;
    }

    Vector2D(float x, float y) {
        u = x;
        v = y;
    }
};

/**
Vector difference
*/
inline void
vector2DDifference(const Vector2D &a, const Vector2D &b, Vector2D &o) {
    o.u = a.u - b.u;
    o.v = a.v - b.v;
}

/**
Square of vector norm: scalar product with itself
*/
inline float
vector2DNorm2(const Vector2D &d) {
    return d.u * d.u + d.v * d.v;
}

#endif
