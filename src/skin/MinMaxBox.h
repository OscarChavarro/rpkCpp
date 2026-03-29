#ifndef __MIN_MAX_BOX__
#define __MIN_MAX_BOX__

#include "common/linealAlgebra/Ray.h"
#include "skin/BoundingBox.h"

class MinMaxBox {
  private:
    BoundingBox boundingBox;
    static inline bool
    clipAxisSlab(
        float minimumBound,
        float maximumBound,
        float origin,
        float direction,
        float toleranceScale,
        float *nearDistance,
        float *farDistance);

  public:
    inline explicit MinMaxBox(const BoundingBox *sourceBoundingBox);
    inline ~MinMaxBox();

    inline MinMaxBox(const MinMaxBox &) = delete;
    inline MinMaxBox &operator=(const MinMaxBox &) = delete;

    void updateFromBoundingBox(const BoundingBox *sourceBoundingBox);
    inline bool intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const;
    inline bool intersectingSegment(const Ray *ray, float *tMin, float *tMax) const;
};

MinMaxBox::MinMaxBox(const BoundingBox *sourceBoundingBox):
        boundingBox()
{
    if ( sourceBoundingBox != nullptr ) {
        boundingBox.copyFrom(sourceBoundingBox);
    }
}

MinMaxBox::~MinMaxBox() {
}

inline bool
MinMaxBox::clipAxisSlab(
        float minimumBound,
        float maximumBound,
        float origin,
        float direction,
        float toleranceScale,
        float *nearDistance,
        float *farDistance) {
    if ( direction == 0.0f ) {
        return !(origin < minimumBound || origin > maximumBound);
    }

    const float invDirection = 1.0f / direction;
    float entryDistance = (minimumBound - origin) * invDirection;
    float exitDistance = (maximumBound - origin) * invDirection;
    if ( entryDistance > exitDistance ) {
        const float swapValue = entryDistance;
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
MinMaxBox::intersectingSegment(const Ray *ray, float *tMin, float *tMax) const {
    if ( ray == nullptr || tMin == nullptr || tMax == nullptr ) {
        return false;
    }

    const float minimumDistance = *tMin;
    const float maximumDistance = *tMax;
    const float *box = boundingBox.rawCoordinates();
    float nearDistance = minimumDistance;
    float farDistance = maximumDistance;
    const float toleranceScale = 1.0f + Numeric::EPSILON_FLOAT;

    if ( !clipAxisSlab(
            box[MIN_X], box[MAX_X],
            ray->position.x, ray->direction.x,
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            box[MIN_Y], box[MAX_Y],
            ray->position.y, ray->direction.y,
            toleranceScale,
            &nearDistance, &farDistance) ) {
        return false;
    }
    if ( !clipAxisSlab(
            box[MIN_Z], box[MAX_Z],
            ray->position.z, ray->direction.z,
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
MinMaxBox::intersect(const Ray *ray, float minimumDistance, float *maximumDistance) const {
    if ( maximumDistance == nullptr ) {
        return false;
    }

    float tMin = minimumDistance;
    float tMax = *maximumDistance;
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
