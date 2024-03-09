#include "common/mymath.h"
#include "skin/BoundingBox.h"

BoundingBox::BoundingBox(): coordinates() {
    coordinates[MIN_X] = HUGE;
    coordinates[MIN_Y] = HUGE;
    coordinates[MIN_Z] = HUGE;
    coordinates[MAX_X] = -HUGE;
    coordinates[MAX_Y] = -HUGE;
    coordinates[MAX_Z] = -HUGE;
}

static void inline
setIfLess(float &a, const float &b) {
    a = a < b ? a : b;
}

static void inline
setIfGreater(float &a, const float &b) {
    a = a > b ? a : b;
}

/**
Enlarge BoundingBox with other
*/
void
BoundingBox::enlarge(const BoundingBox *other) {
    setIfLess(coordinates[MIN_X], other->coordinates[MIN_X]);
    setIfLess(coordinates[MIN_Y], other->coordinates[MIN_Y]);
    setIfLess(coordinates[MIN_Z], other->coordinates[MIN_Z]);
    setIfGreater(coordinates[MAX_X], other->coordinates[MAX_X]);
    setIfGreater(coordinates[MAX_Y], other->coordinates[MAX_Y]);
    setIfGreater(coordinates[MAX_Z], other->coordinates[MAX_Z]);
}

void
BoundingBox::enlargeToIncludePoint(const Vector3D *point) {
    setIfLess(coordinates[MIN_X], point->x);
    setIfLess(coordinates[MIN_Y], point->y);
    setIfLess(coordinates[MIN_Z], point->z);
    setIfGreater(coordinates[MAX_X], point->x);
    setIfGreater(coordinates[MAX_Y], point->y);
    setIfGreater(coordinates[MAX_Z], point->z);
}

void
BoundingBox::copyFrom(const BoundingBox *other) {
    coordinates[MIN_X] = other->coordinates[MIN_X];
    coordinates[MIN_Y] = other->coordinates[MIN_Y];
    coordinates[MIN_Z] = other->coordinates[MIN_Z];
    coordinates[MAX_X] = other->coordinates[MAX_X];
    coordinates[MAX_Y] = other->coordinates[MAX_Y];
    coordinates[MAX_Z] = other->coordinates[MAX_Z];
}

/**
Check for intersection between bounding box and the given ray.
If there is an intersection between minimum distance and *maximum distance along
the ray, *maximum distance is replaced with the distance to the point of
intersection, and true is returned.  Otherwise, false is returned.

If this routine is used to check for intersection with a volume
rather than a "hollow" box, one should first determine if
(ray->pos + minimum distance * ray->dir) is inside the bounding volume, and
call intersect() only if it is not.

This routine was taken from ray-shade [PhB].
*/

/**
This routine computes the segment of intersection of the ray and the bounding box.
On input, tMin and tMax contain minimum and maximum allowed distance to the ray
origin. On output, tMin and tMin contain the distance to the eye origin of
the intersection positions of the ray with the bounding box.
If there are no intersection in the given interval, false is returned. If there
are intersections, true is returned.
*/
bool
BoundingBox::intersectingSegment(Ray *ray, float *tMin, float *tMax) const {
    float t;

    float minimumDistance = *tMin;
    float maximumDistance = *tMax;

    float dir = ray->dir.x;
    float pos = ray->pos.x;

    if ( dir < 0 ) {
        t = (coordinates[MIN_X] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MAX_X] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0 ) {
        t = (coordinates[MAX_X] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MIN_X] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0 + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < coordinates[MIN_X] || pos > coordinates[MAX_X] ) {
        return false;
    }

    dir = ray->dir.y;
    pos = ray->pos.y;

    if ( dir < 0 ) {
        t = (coordinates[MIN_Y] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MAX_Y] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0 + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0 ) {
        t = (coordinates[MAX_Y] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MIN_Y] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < coordinates[MIN_Y] || pos > coordinates[MAX_Y] ) {
        return false;
    }

    dir = ray->dir.z;
    pos = ray->pos.z;

    if ( dir < 0 ) {
        t = (coordinates[MIN_Z] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MAX_Z] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0 + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0 ) {
        t = (coordinates[MAX_Z] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (coordinates[MIN_Z] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0 + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < coordinates[MIN_Z] || pos > coordinates[MAX_Z] ) {
        return false;
    }

    // If *tMin == minDist, then there was no "near"
    // intersection farther than EPSILON away
    if ( *tMin == minimumDistance ) {
        if ( *tMax < maximumDistance ) {
            return true;
        }
    } else {
        if ( *tMin < maximumDistance ) {
            return true;
        }
    }
    return false; // Hit, but not closer than maximumDistance
}

bool
BoundingBox::intersect(Ray *ray, float minimumDistance, float *maximumDistance) const {
    float tMin = minimumDistance;
    float tMax = *maximumDistance;
    int hit = intersectingSegment(ray, &tMin, &tMax);
    if ( hit ) {
        // Reduce maximumDistance
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

/**
Returns true if the bounding box is behind the plane defined by norm and d
see F. Tampieri, Fast Vertex Radiosity Update, Graphics Gems II, p 303
*/
bool
BoundingBox::behindPlane(Vector3D *norm, float d) const {
    Vector3D P;

    if ( norm->x > 0.0f ) {
        P.x = coordinates[MAX_X];
    } else {
        P.x = coordinates[MIN_X];
    }

    if ( norm->y > 0.0f ) {
        P.y = coordinates[MAX_Y];
    } else {
        P.y = coordinates[MIN_Y];
    }

    if ( norm->z > 0.0f ) {
        P.z = coordinates[MAX_Z];
    } else {
        P.z = coordinates[MIN_Z];
    }

    return vectorDotProduct(*norm, P) + d <= 0.0;
}

/**
Computes bounding box after transforming this with coordinates after applying transform.
Result is filled in transBoundingBox->coordinates
*/
void
BoundingBox::transformTo(Matrix4x4 *transform, BoundingBox *transformedBoundingBox) {
    Vector3D v[8];

    vectorSet(v[0], coordinates[MIN_X], coordinates[MIN_Y], coordinates[MIN_Z]);
    vectorSet(v[1], coordinates[MAX_X], coordinates[MIN_Y], coordinates[MIN_Z]);
    vectorSet(v[2], coordinates[MIN_X], coordinates[MAX_Y], coordinates[MIN_Z]);
    vectorSet(v[3], coordinates[MAX_X], coordinates[MAX_Y], coordinates[MIN_Z]);
    vectorSet(v[4], coordinates[MIN_X], coordinates[MIN_Y], coordinates[MAX_Z]);
    vectorSet(v[5], coordinates[MAX_X], coordinates[MIN_Y], coordinates[MAX_Z]);
    vectorSet(v[6], coordinates[MIN_X], coordinates[MAX_Y], coordinates[MAX_Z]);
    vectorSet(v[7], coordinates[MAX_X], coordinates[MAX_Y], coordinates[MAX_Z]);

    for ( int i = 0; i < 8; i++ ) {
        transformPoint3D(*transform, v[i], v[i]);
        transformedBoundingBox->enlargeToIncludePoint(&v[i]);
    }

    float d;
    d = (transformedBoundingBox->coordinates[MAX_X] - transformedBoundingBox->coordinates[MIN_X]) * (float)EPSILON;
    transformedBoundingBox->coordinates[MIN_X] -= d;
    transformedBoundingBox->coordinates[MAX_X] += d;
    d = (transformedBoundingBox->coordinates[MAX_Y] - transformedBoundingBox->coordinates[MIN_Y]) * (float)EPSILON;
    transformedBoundingBox->coordinates[MIN_Y] -= d;
    transformedBoundingBox->coordinates[MAX_Y] += d;
    d = (transformedBoundingBox->coordinates[MAX_Z] - transformedBoundingBox->coordinates[MIN_Z]) * (float)EPSILON;
    transformedBoundingBox->coordinates[MIN_Z] -= d;
    transformedBoundingBox->coordinates[MAX_Z] += d;
}
