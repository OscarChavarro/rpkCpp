#ifndef __RPK_AXIS_ALIGNED_BOX__
#define __RPK_AXIS_ALIGNED_BOX__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/skin/BoundingBoxCoordinateIndex.h"

/**
A bounding box is represented as an array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X
*/
class RpkAxisAlignedBoundingBox {
  private:
    double coordinates[6];

    static void inline
    setIfLess(double &a, double b) {
        a = a < b ? a : b;
    }

    static void inline
    setIfGreater(double &a, double b) {
        a = a > b ? a : b;
    }

    static bool inline
    clipAxisSlab(
        double minimumBound,
        double maximumBound,
        double origin,
        double direction,
        double toleranceScale,
        double *nearDistance,
        double *farDistance);

  public:
    RpkAxisAlignedBoundingBox();

    double maxExtent() const;
    bool outOfBounds(const Vector3Dd *p) const;
    Vector3Dd center() const;
    void setAsUnion(const RpkAxisAlignedBoundingBox *a, const RpkAxisAlignedBoundingBox *b);
    bool disjointToOtherBoundingBox(const RpkAxisAlignedBoundingBox *other) const;
    bool behindPlane(const Vector3Dd &normal, double distance) const;
    void copyFrom(const RpkAxisAlignedBoundingBox *other);
    void enlarge(const RpkAxisAlignedBoundingBox *other);
    void enlargeToIncludePoint(const Vector3Dd *point);
    void enlargeTinyBit();
    void computeContributionFlags(const RpkAxisAlignedBoundingBox *other, bool *hasMinMax1, bool *hasMinMax2) const;
    double dx() const;
    double dy() const;
    double dz() const;
    Vector3Dd voxelSize(const short nx, const short ny, const short nz) const;
    void enlargeByFactor(double factor);
    Vector3Dd minPoint() const;
    Vector3Dd maxPoint() const;
    double minX() const;
    double minY() const;
    double minZ() const;
    double maxX() const;
    double maxY() const;
    double maxZ() const;
    const double *rawCoordinates() const;
    double valueAt(int idx) const;
    void corners(Vector3Dd out[8]) const;
    bool intersect(const Ray *ray, double minimumDistance, double *maximumDistance) const;
    bool intersectingSegment(const Ray *ray, double *tMin, double *tMax) const;
};

inline bool
RpkAxisAlignedBoundingBox::clipAxisSlab(
    double minimumBound,
    double maximumBound,
    double origin,
    double direction,
    double toleranceScale,
    double *nearDistance,
    double *farDistance) {
    if ( direction == 0.0 ) {
        return !(origin < minimumBound || origin > maximumBound);
    }

    const double invDirection = 1.0 / direction;
    double entryDistance = (minimumBound - origin) * invDirection;
    double exitDistance = (maximumBound - origin) * invDirection;
    if ( entryDistance > exitDistance ) {
        const double swapValue = entryDistance;
        entryDistance = exitDistance;
        exitDistance = swapValue;
    }

    if ( exitDistance < *nearDistance ) {
        return false;
    }
    if ( entryDistance > *nearDistance ) {
        *nearDistance = entryDistance;
    }
    if ( exitDistance < *farDistance ) {
        *farDistance = exitDistance;
    }
    return *nearDistance <= (*farDistance * toleranceScale);
}

inline bool
RpkAxisAlignedBoundingBox::intersectingSegment(const Ray *ray, double *tMin, double *tMax) const {
    if ( ray == NULL || tMin == NULL || tMax == NULL ) {
        return false;
    }

    const double minimumDistance = *tMin;
    const double maximumDistance = *tMax;
    double nearDistance = minimumDistance;
    double farDistance = maximumDistance;
    const double toleranceScale = 1.0 + Numeric::EPSILON;

    if ( !clipAxisSlab(
            coordinates[MIN_X], coordinates[MAX_X],
            ray->getOrigin().x(), ray->getDirection().x(),
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            coordinates[MIN_Y], coordinates[MAX_Y],
            ray->getOrigin().y(), ray->getDirection().y(),
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            coordinates[MIN_Z], coordinates[MAX_Z],
            ray->getOrigin().z(), ray->getDirection().z(),
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }

    *tMin = nearDistance;
    *tMax = farDistance;

    if ( nearDistance == minimumDistance ) {
        if ( farDistance < maximumDistance ) {
            return true;
        }
    } else {
        if ( nearDistance < maximumDistance ) {
            return true;
        }
    }
    return false;
}

inline bool
RpkAxisAlignedBoundingBox::intersect(const Ray *ray, double minimumDistance, double *maximumDistance) const {
    if ( maximumDistance == NULL ) {
        return false;
    }

    double tMin = minimumDistance;
    double tMax = *maximumDistance;
    const bool hit = intersectingSegment(ray, &tMin, &tMax);
    if ( hit ) {
        if ( tMin == minimumDistance ) {
            if ( tMax < *maximumDistance ) {
                *maximumDistance = tMax;
            }
        } else {
            if ( tMin < *maximumDistance ) {
                *maximumDistance = tMin;
            }
        }
    }
    return hit;
}

inline double
RpkAxisAlignedBoundingBox::maxExtent() const {
    const double dx = coordinates[MAX_X] - coordinates[MIN_X];
    const double dy = coordinates[MAX_Y] - coordinates[MIN_Y];
    const double dz = coordinates[MAX_Z] - coordinates[MIN_Z];
    return dx > dy
           ? (dx > dz ? dx : dz)
           : (dy > dz ? dy : dz);
}

inline bool
RpkAxisAlignedBoundingBox::outOfBounds(const Vector3Dd *p) const {
    return p->x() < coordinates[MIN_X] || p->x() > coordinates[MAX_X] ||
           p->y() < coordinates[MIN_Y] || p->y() > coordinates[MAX_Y] ||
           p->z() < coordinates[MIN_Z] || p->z() > coordinates[MAX_Z];
}

inline Vector3Dd
RpkAxisAlignedBoundingBox::center() const {
    return Vector3Dd(
        0.5 * (coordinates[MIN_X] + coordinates[MAX_X]),
        0.5 * (coordinates[MIN_Y] + coordinates[MAX_Y]),
        0.5 * (coordinates[MIN_Z] + coordinates[MAX_Z])
    );
}

inline void
RpkAxisAlignedBoundingBox::setAsUnion(const RpkAxisAlignedBoundingBox *a, const RpkAxisAlignedBoundingBox *b) {
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
RpkAxisAlignedBoundingBox::disjointToOtherBoundingBox(const RpkAxisAlignedBoundingBox *other) const {
    return
            (coordinates[MIN_X] > other->coordinates[MAX_X]) || (other->coordinates[MIN_X] > coordinates[MAX_X]) ||
            (coordinates[MIN_Y] > other->coordinates[MAX_Y]) || (other->coordinates[MIN_Y] > coordinates[MAX_Y]) ||
            (coordinates[MIN_Z] > other->coordinates[MAX_Z]) || (other->coordinates[MIN_Z] > coordinates[MAX_Z]);
}

inline void
RpkAxisAlignedBoundingBox::computeContributionFlags(const RpkAxisAlignedBoundingBox *other, bool *hasMinMax1, bool *hasMinMax2) const {
    for (int i = MIN_X; i <= MIN_Z; i++) {
        if ( coordinates[i] < other->coordinates[i]) {
            hasMinMax1[i] = true;
        } else {
            if (!Numeric::doubleEqual(
                    coordinates[i],
                    other->coordinates[i],
                    VSDK::EPSILON)) {
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
                    VSDK::EPSILON)) {
                hasMinMax2[i] = true;
            }
        }
    }
}

inline double
RpkAxisAlignedBoundingBox::dx() const {
    return coordinates[MAX_X] - coordinates[MIN_X];
}

inline double
RpkAxisAlignedBoundingBox::dy() const {
    return coordinates[MAX_Y] - coordinates[MIN_Y];
}

inline double
RpkAxisAlignedBoundingBox::dz() const {
    return coordinates[MAX_Z] - coordinates[MIN_Z];
}

inline Vector3Dd
RpkAxisAlignedBoundingBox::voxelSize(const short nx, const short ny, const short nz) const {
    return Vector3Dd(
            dx() / ((double)(nx)),
            dy() / ((double)(ny)),
            dz() / ((double)(nz))
    );
}

inline void
RpkAxisAlignedBoundingBox::enlargeByFactor(double factor) {
    double fdx = dx() * factor;
    double fdy = dy() * factor;
    double fdz = dz() * factor;

    if (fdx < Numeric::EPSILON) {
        fdx = Numeric::EPSILON;
    }
    if (fdy < Numeric::EPSILON) {
        fdy = Numeric::EPSILON;
    }
    if (fdz < Numeric::EPSILON) {
        fdz = Numeric::EPSILON;
    }

    coordinates[MIN_X] -= fdx;
    coordinates[MAX_X] += fdx;
    coordinates[MIN_Y] -= fdy;
    coordinates[MAX_Y] += fdy;
    coordinates[MIN_Z] -= fdz;
    coordinates[MAX_Z] += fdz;
}

inline Vector3Dd
RpkAxisAlignedBoundingBox::minPoint() const {
    return Vector3Dd(
            coordinates[MIN_X],
            coordinates[MIN_Y],
            coordinates[MIN_Z]
    );
}

inline Vector3Dd
RpkAxisAlignedBoundingBox::maxPoint() const {
    return Vector3Dd(
            coordinates[MAX_X],
            coordinates[MAX_Y],
            coordinates[MAX_Z]
    );
}

inline double
RpkAxisAlignedBoundingBox::minX() const {
    return coordinates[MIN_X];
}

inline double
RpkAxisAlignedBoundingBox::minY() const {
    return coordinates[MIN_Y];
}

inline double
RpkAxisAlignedBoundingBox::minZ() const {
    return coordinates[MIN_Z];
}

inline double
RpkAxisAlignedBoundingBox::maxX() const {
    return coordinates[MAX_X];
}

inline double
RpkAxisAlignedBoundingBox::maxY() const {
    return coordinates[MAX_Y];
}

inline double
RpkAxisAlignedBoundingBox::maxZ() const {
    return coordinates[MAX_Z];
}

inline const double *
RpkAxisAlignedBoundingBox::rawCoordinates() const {
    return coordinates;
}

inline double
RpkAxisAlignedBoundingBox::valueAt(int idx) const {
    switch (idx) {
        case MIN_X: return minX();
        case MIN_Y: return minY();
        case MIN_Z: return minZ();
        case MAX_X: return maxX();
        case MAX_Y: return maxY();
        case MAX_Z: return maxZ();
        default: return 0.0;
    }
}

inline void
RpkAxisAlignedBoundingBox::corners(Vector3Dd out[8]) const {
    const double minX = coordinates[MIN_X];
    const double minY = coordinates[MIN_Y];
    const double minZ = coordinates[MIN_Z];
    const double maxX = coordinates[MAX_X];
    const double maxY = coordinates[MAX_Y];
    const double maxZ = coordinates[MAX_Z];

    out[0] = Vector3Dd(minX, minY, minZ);
    out[1] = Vector3Dd(maxX, minY, minZ);
    out[2] = Vector3Dd(minX, maxY, minZ);
    out[3] = Vector3Dd(maxX, maxY, minZ);
    out[4] = Vector3Dd(minX, minY, maxZ);
    out[5] = Vector3Dd(maxX, minY, maxZ);
    out[6] = Vector3Dd(minX, maxY, maxZ);
    out[7] = Vector3Dd(maxX, maxY, maxZ);
}

#endif
