#ifndef __VECTOR3DD__
#define __VECTOR3DD__

// Should be changed to Vector3Dd
//typedef double VECTOR3Dd[3];

class VECTOR3Dd {
  public:
    double x;
    double y;
    double z;

    VECTOR3Dd();
    VECTOR3Dd(double inX, double inY, double inZ);
};

extern double normalize(VECTOR3Dd *v, double epsilon);
extern void floatCrossProduct(VECTOR3Dd *result, const VECTOR3Dd *a, const VECTOR3Dd *b);
extern double distanceSquared(const VECTOR3Dd *v1, const VECTOR3Dd *v2);

inline void
mgfVertexCopy(VECTOR3Dd *result, const VECTOR3Dd *source) {
    result->x = source->x;
    result->y = source->y;
    result->z = source->z;
}

extern inline double
dotProduct(const VECTOR3Dd *a, const VECTOR3Dd *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

extern inline bool
is0Vector(const VECTOR3Dd *v, double epsilon) {
    return dotProduct(v, v) <= epsilon * epsilon;
}

extern inline void
round0(double &x, double epsilon) {
    if ( x <= epsilon && x >= -epsilon ) {
        x = 0;
    }
}

#endif
