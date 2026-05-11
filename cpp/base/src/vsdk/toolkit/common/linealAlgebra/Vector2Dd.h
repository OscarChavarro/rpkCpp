#ifndef VECTOR_2DD__
#define VECTOR_2DD__

class Vector2Dd {
  public:
    double u;
    double v;

    Vector2Dd();

    static void set(Vector2Dd &v, double a, double b);
    static void subtract(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r);
    static void add(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r);
    static void negate(Vector2Dd &p);
    static double determinant(const Vector2Dd &A, const Vector2Dd &B);
};

inline
Vector2Dd::Vector2Dd() {
    u = 0.0;
    v = 0.0;
}

inline
void Vector2Dd::set(Vector2Dd &v, double a, double b) {
    v.u = a;
    v.v = b;
}

inline void
Vector2Dd::subtract(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u - q.u;
    r.v = p.v - q.v;
}

inline void
Vector2Dd::add(const Vector2Dd &p, const Vector2Dd &q, Vector2Dd &r) {
    r.u = p.u + q.u;
    r.v = p.v + q.v;
}

inline void
Vector2Dd::negate(Vector2Dd &p) {
    p.u = -p.u;
    p.v = -p.v;
}

inline double
Vector2Dd::determinant(const Vector2Dd &A, const Vector2Dd &B) {
    return (A.u * B.v - A.v * B.u);
}

#endif
