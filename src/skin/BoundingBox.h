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
  private:
    void inline
    setIfLess(float &a, const float &b) {
        a = a < b ? a : b;
    }

    void inline
    setIfGreater(float &a, const float &b) {
        a = a > b ? a : b;
    }

  public:
    float coordinates[6];

    BoundingBox();

    inline float
    maxExtent() const {
        const float dx = coordinates[MAX_X] - coordinates[MIN_X];
        const float dy = coordinates[MAX_Y] - coordinates[MIN_Y];
        const float dz = coordinates[MAX_Z] - coordinates[MIN_Z];
        return dx > dy
           ? (dx > dz ? dx : dz)
           : (dy > dz ? dy : dz);
    }

    inline Matrix4x4
    createOrthographicProjectionMatrix() const {
        return Matrix4x4::createOrthogonalViewMatrix(
                coordinates[MIN_X],
                coordinates[MAX_X],
                coordinates[MIN_Y],
                coordinates[MAX_Y],
                -coordinates[MAX_Z],
                -coordinates[MIN_Z]
        );
    }

    inline bool
    outOfBounds(const Vector3D *p) const {
        return p->x < coordinates[MIN_X] || p->x > coordinates[MAX_X] ||
               p->y < coordinates[MIN_Y] || p->y > coordinates[MAX_Y] ||
               p->z < coordinates[MIN_Z] || p->z > coordinates[MAX_Z];
    }

    inline Vector3D
    center() const {
        return Vector3D(
          0.5f * (coordinates[MIN_X] + coordinates[MAX_X]),
          0.5f * (coordinates[MIN_Y] + coordinates[MAX_Y]),
          0.5f * (coordinates[MIN_Z] + coordinates[MAX_Z])
        );
    }

    inline void
    setAsUnion(const BoundingBox *a, const BoundingBox *b) {
        for (int i = MIN_X; i <= MIN_Z; i++) {
            coordinates[i] = a->coordinates[i] < b->coordinates[i]
                             ? a->coordinates[i]
                             : b->coordinates[i];
        }

        for (int i = MAX_X; i <= MAX_Z; i++) {
            coordinates[i] = a->coordinates[i] > b->coordinates[i]
                             ? a->coordinates[i]
                             : b->coordinates[i];
        }
    }

    /**
    True if the two given bounding boxes are disjoint
    */
    inline bool
    disjointToOtherBoundingBox(const BoundingBox *other) const {
        return
            (coordinates[MIN_X] > other->coordinates[MAX_X]) || (other->coordinates[MIN_X] > coordinates[MAX_X]) ||
            (coordinates[MIN_Y] > other->coordinates[MAX_Y]) || (other->coordinates[MIN_Y] > coordinates[MAX_Y]) ||
            (coordinates[MIN_Z] > other->coordinates[MAX_Z]) || (other->coordinates[MIN_Z] > coordinates[MAX_Z]);
    }

    bool intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const;
    bool intersectingSegment(const Ray *ray, float *tMin, float *tMax) const;
    bool behindPlane(const Vector3D *normal, float distance) const;
    void copyFrom(const BoundingBox *other);
    void enlarge(const BoundingBox *other);
    void enlargeToIncludePoint(const Vector3D *point);
    void transformTo(const Matrix4x4 *transform, BoundingBox *transformedBoundingBox) const;
    void enlargeTinyBit();

    inline void
    computeContributionFlags(
        const BoundingBox *other,
        bool *hasMinMax1,
        bool *hasMinMax2
    ) {
        for (int i = MIN_X; i <= MIN_Z; i++) {
            if ( coordinates[i] < other->coordinates[i]) {
                hasMinMax1[i] = true;
            } else {
                if (!Numeric::doubleEqual(
                        coordinates[i],
                        other->coordinates[i],
                        Numeric::EPSILON)) {
                    hasMinMax2[i] = true;
                }
            }
        }

        for (int i = MAX_X; i <= MAX_Z; i++) {
            if ( coordinates[i] > other->coordinates[i]) {
                hasMinMax1[i] = true;
            } else {
                if (!Numeric::doubleEqual(
                        coordinates[i],
                        other->coordinates[i],
                        Numeric::EPSILON)) {
                    hasMinMax2[i] = true;
                }
            }
        }
    }
};

#endif
