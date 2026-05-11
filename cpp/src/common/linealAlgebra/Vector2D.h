#ifndef VECTOR_2D__
#define VECTOR_2D__

class Vector2D {
  public:
    float u;
    float v;

    Vector2D();
    Vector2D(float x, float y);

    static void difference(const Vector2D &a, const Vector2D &b, Vector2D &o);
    static float norm2(const Vector2D &d);
};

inline
Vector2D::Vector2D() {
    u = 0.0;
    v = 0.0;
}

inline
Vector2D::Vector2D(float x, float y) {
    u = x;
    v = y;
}

/**
Vector difference
*/
inline void
Vector2D::difference(const Vector2D &a, const Vector2D &b, Vector2D &o) {
    o.u = a.u - b.u;
    o.v = a.v - b.v;
}

/**
Square of vector norm: scalar product with itself
*/
inline float
Vector2D::norm2(const Vector2D &d) {
    return d.u * d.u + d.v * d.v;
}

#endif
