#ifndef __BOUNDING_BOX__
#define __BOUNDING_BOX__

#include "common/linealAlgebra/Matrix4x4.h"
#include "common/Ray.h"

/**
The following defines must obey the following rules:
1. (MIN_X + 3) = MAX_X and (MAX_X + 3) = MIN_X and same for MIN_Y, ...
2. MIN_X + 1 = MIN_Y, MIN_Y + 1 = MIN_Z and MAX_X + 1 = MAX_Y, MAX_Y+1 = MAX_Z
*/
#define MIN_X 0
#define MIN_Y 1
#define MIN_Z 2
#define MAX_X 3
#define MAX_Y 4
#define MAX_Z 5

/**
A bounding box is represented as an array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X
*/
class BoundingBox {
  public:
    float coordinates[6];
    BoundingBox();

    inline bool
    outOfBounds(const Vector3D *p) const {
        return p->x < coordinates[MIN_X] || p->x > coordinates[MAX_X] ||
               p->y < coordinates[MIN_Y] || p->y > coordinates[MAX_Y] ||
               p->z < coordinates[MIN_Z] || p->z > coordinates[MAX_Z];
    }

    /**
    True if the two given bounding boxes are disjoint
    */
    inline bool
    disjointToOtherBoundingBox(const BoundingBox *other) {
        return
            (coordinates[MIN_X] > other->coordinates[MAX_X]) || (other->coordinates[MIN_X] > coordinates[MAX_X]) ||
            (coordinates[MIN_Y] > other->coordinates[MAX_Y]) || (other->coordinates[MIN_Y] > coordinates[MAX_Y]) ||
            (coordinates[MIN_Z] > other->coordinates[MAX_Z]) || (other->coordinates[MIN_Z] > coordinates[MAX_Z]);
    }

    bool intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const;
    bool intersectingSegment(const Ray *ray, float *tMin, float *tMax) const;
    bool behindPlane(const Vector3D *norm, float d) const;
    void copyFrom(const BoundingBox *other);
    void enlarge(const BoundingBox *other);
    void enlargeToIncludePoint(const Vector3D *point);
    void transformTo(const Matrix4x4 *transform, BoundingBox *transformedBoundingBox) const;
    void enlargeTinyBit();
};

#endif
