#include "skin/MinMaxBox.h"

MinMaxBox::MinMaxBox(const BoundingBox *sourceBoundingBox):
    boundingBox()
{
    if ( sourceBoundingBox != nullptr ) {
        boundingBox.copyFrom(sourceBoundingBox);
    }
}

void
MinMaxBox::updateFromBoundingBox(const BoundingBox *sourceBoundingBox) {
    if ( sourceBoundingBox == nullptr ) {
        return;
    }
    boundingBox.copyFrom(sourceBoundingBox);
}

bool
MinMaxBox::intersectingSegment(const Ray *ray, float *tMin, float *tMax) const {
    if ( ray == nullptr || tMin == nullptr || tMax == nullptr ) {
        return false;
    }

    const float minimumDistance = *tMin;
    const float maximumDistance = *tMax;
    const float minX = boundingBox.minX();
    const float minY = boundingBox.minY();
    const float minZ = boundingBox.minZ();
    const float maxX = boundingBox.maxX();
    const float maxY = boundingBox.maxY();
    const float maxZ = boundingBox.maxZ();
    float t;

    float dir = ray->direction.x;
    float pos = ray->position.x;
    if ( dir < 0.0f ) {
        t = (minX - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (maxX - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0f ) {
        t = (maxX - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (minX - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < minX || pos > maxX ) {
        return false;
    }

    dir = ray->direction.y;
    pos = ray->position.y;
    if ( dir < 0.0f ) {
        t = (minY - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (maxY - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0f ) {
        t = (maxY - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (minY - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < minY || pos > maxY ) {
        return false;
    }

    dir = ray->direction.z;
    pos = ray->position.z;
    if ( dir < 0.0f ) {
        t = (minZ - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (maxZ - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0f ) {
        t = (maxZ - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (minZ - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0f + Numeric::EPSILON_FLOAT) ) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < minZ || pos > maxZ ) {
        return false;
    }

    if ( *tMin == minimumDistance ) {
        if ( *tMax < maximumDistance ) {
            return true;
        }
    } else {
        if ( *tMin < maximumDistance ) {
            return true;
        }
    }
    return false;
}

bool
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
