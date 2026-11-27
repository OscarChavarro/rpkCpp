#ifndef __MIN_MAX_BOX__
#define __MIN_MAX_BOX__

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"

class MinMaxBox {
  private:
    RpkAxisAlignedBoundingBox boundingBox;
    static inline bool
    clipAxisSlab(
        double minimumBound,
        double maximumBound,
        double origin,
        double direction,
        double toleranceScale,
        double *nearDistance,
        double *farDistance);

  public:
    inline explicit MinMaxBox(const RpkAxisAlignedBoundingBox *sourceBoundingBox);
    inline ~MinMaxBox();

    inline MinMaxBox(const MinMaxBox &);
    inline MinMaxBox &operator=(const MinMaxBox &);

    void updateFromBoundingBox(const RpkAxisAlignedBoundingBox *sourceBoundingBox);
    inline bool intersect(const Ray *ray, double minimumDistance, double *maximumDistance) const;
    inline bool intersectingSegment(const Ray *ray, double *tMin, double *tMax) const;
};

MinMaxBox::MinMaxBox(const RpkAxisAlignedBoundingBox *sourceBoundingBox):
        boundingBox()
{
    if ( sourceBoundingBox != NULL ) {
        boundingBox.copyFrom(sourceBoundingBox);
    }
}

MinMaxBox::~MinMaxBox() {
}

inline bool
MinMaxBox::clipAxisSlab(
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
MinMaxBox::intersectingSegment(const Ray *ray, double *tMin, double *tMax) const {
    if ( ray == NULL || tMin == NULL || tMax == NULL ) {
        return false;
    }

    const double minimumDistance = *tMin;
    const double maximumDistance = *tMax;
    const double *box = boundingBox.rawCoordinates();
    double nearDistance = minimumDistance;
    double farDistance = maximumDistance;
    const double toleranceScale = 1.0 + Numeric::EPSILON;

    if ( !clipAxisSlab(
            box[MIN_X], box[MAX_X],
            ray->getOrigin().x(), ray->getDirection().x(),
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            box[MIN_Y], box[MAX_Y],
            ray->getOrigin().y(), ray->getDirection().y(),
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            box[MIN_Z], box[MAX_Z],
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
MinMaxBox::intersect(const Ray *ray, double minimumDistance, double *maximumDistance) const {
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

#endif
