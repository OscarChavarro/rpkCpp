#ifndef __VECTOR_3D_D__
#define __VECTOR_3D_D__

class Vector3Dd {
  public:
    double x;
    double y;
    double z;

    Vector3Dd();
    Vector3Dd(double inX, double inY, double inZ);
    virtual ~Vector3Dd();

    double distanceSquared(const Vector3Dd *v2) const;
    double dotProduct(const Vector3Dd *b) const;
    bool isNull(double epsilon) const;

    // TODO: Replace this odd method with standard norm and normalize operations
    double normalizeAndGivePreviousNorm(double epsilon);
    void crossProduct(const Vector3Dd *a, const Vector3Dd *b);
    void copy(const Vector3Dd *source);
};

inline bool
Vector3Dd::isNull(double epsilon) const {
    return dotProduct(this) <= epsilon * epsilon;
}

inline double
Vector3Dd::dotProduct(const Vector3Dd *b) const {
    return x * b->x + y * b->y + z * b->z;
}

inline void
Vector3Dd::copy(const Vector3Dd *source) {
    x = source->x;
    y = source->y;
    z = source->z;
}

#endif
