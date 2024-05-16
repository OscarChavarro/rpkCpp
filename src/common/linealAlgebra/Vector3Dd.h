#ifndef __VECTOR_3D_D__
#define __VECTOR_3D_D__

class VECTOR3Dd {
  public:
    double x;
    double y;
    double z;

    VECTOR3Dd();
    VECTOR3Dd(double inX, double inY, double inZ);
    virtual ~VECTOR3Dd();

    double distanceSquared(const VECTOR3Dd *v2) const;

    double
    dotProduct(const VECTOR3Dd *b) const {
        return x * b->x + y * b->y + z * b->z;
    }

    inline bool
    isNull(double epsilon) const {
        return dotProduct(this) <= epsilon * epsilon;
    }

    // TODO: Replace this odd method with standard norm and normalize operations
    double normalizeAndGivePreviousNorm(double epsilon);
    void crossProduct(const VECTOR3Dd *a, const VECTOR3Dd *b);

    void
    copy(const VECTOR3Dd *source) {
        x = source->x;
        y = source->y;
        z = source->z;
    }
};

#endif
