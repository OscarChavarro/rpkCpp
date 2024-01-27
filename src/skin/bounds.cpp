#include <cstdlib>

#include "common/mymath.h"
#include "skin/bounds.h"

static void inline
setIfLess(float &a, float &b) {
    a = a < b ? a : b;
}

static void inline
setIfGreater(float &a, float &b) {
    a = a > b ? a : b;
}

float *
boundsCreate() {
    float *bounds;

    bounds = (float *)malloc(sizeof(BOUNDINGBOX));
    return bounds;
}

void
boundsDestroy(float *bounds) {
    free(bounds);
}

float *
boundsInit(float *bounds) {
    bounds[MIN_X] = bounds[MIN_Y] = bounds[MIN_Z] = HUGE;
    bounds[MAX_X] = bounds[MAX_Y] = bounds[MAX_Z] = -HUGE;
    return bounds;
}

/**
Enlarge getBoundingBox with extra
*/
float *
boundsEnlarge(float *bounds, float *extra) {
    setIfLess(bounds[MIN_X], extra[MIN_X]);
    setIfLess(bounds[MIN_Y], extra[MIN_Y]);
    setIfLess(bounds[MIN_Z], extra[MIN_Z]);
    setIfGreater(bounds[MAX_X], extra[MAX_X]);
    setIfGreater(bounds[MAX_Y], extra[MAX_Y]);
    setIfGreater(bounds[MAX_Z], extra[MAX_Z]);
    return bounds;
}

float *
boundsEnlargePoint(float *bounds, Vector3D *point) {
    setIfLess(bounds[MIN_X], point->x);
    setIfLess(bounds[MIN_Y], point->y);
    setIfLess(bounds[MIN_Z], point->z);
    setIfGreater(bounds[MAX_X], point->x);
    setIfGreater(bounds[MAX_Y], point->y);
    setIfGreater(bounds[MAX_Z], point->z);
    return bounds;
}

float *
boundsCopy(const float *from, float *to) {
    to[MIN_X] = from[MIN_X];
    to[MIN_Y] = from[MIN_Y];
    to[MIN_Z] = from[MIN_Z];
    to[MAX_X] = from[MAX_X];
    to[MAX_Y] = from[MAX_Y];
    to[MAX_Z] = from[MAX_Z];
    return to;
}

/**
Check for intersection between bounding box and the given ray.
If there is an intersection between mindist and *maxdist along
the ray, *maxdist is replaced with the distance to the point of
intersection, and true is returned.  Otherwise, false is returned.

If this routine is used to check for intersection with a volume
rather than a "hollow" box, one should first determine if
(ray->pos + mindist * ray->dir) is inside the bounding volume, and
call boundsIntersect() only if it is not.

This routine was taken from rayshade [PhB].
*/

/**
This routine computes the segment of intersection of the ray and the bounding box.
On input, tmin and tmax contain minimum and maximum allowed distance to the ray 
origin. On output, tmin and tmin contain the distance to the eye origin of
the intersection positions of the ray with the boundingbox.
If there are no intersection in the given interval, false is returned. If there
are intersections, true is returned.
*/
int
boundsIntersectingSegment(Ray *ray, const float *bounds, float *tMin, float *tMax) {
    float t;
    float minimumDistance;
    float maximumDistance;
    float dir;
    float pos;

    minimumDistance = *tMin;
    maximumDistance = *tMax;

    dir = ray->dir.x;
    pos = ray->pos.x;

    if ( dir < 0 ) {
        t = (bounds[MIN_X] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MAX_X] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0.0 ) {
        t = (bounds[MAX_X] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MIN_X] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1.0 + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < bounds[MIN_X] || pos > bounds[MAX_X] ) {
        return false;
    }

    dir = ray->dir.y;
    pos = ray->pos.y;

    if ( dir < 0 ) {
        t = (bounds[MIN_Y] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MAX_Y] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0. ) {
        t = (bounds[MAX_Y] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MIN_Y] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < bounds[MIN_Y] || pos > bounds[MAX_Y] ) {
        return false;
    }

    dir = ray->dir.z;
    pos = ray->pos.z;

    if ( dir < 0 ) {
        t = (bounds[MIN_Z] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MAX_Z] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( dir > 0. ) {
        t = (bounds[MAX_Z] - pos) / dir;
        if ( t < *tMin ) {
            return false;
        }
        if ( t <= *tMax ) {
            *tMax = t;
        }
        t = (bounds[MIN_Z] - pos) / dir;
        if ( t >= *tMin ) {
            if ( t > *tMax * (1. + EPSILON)) {
                return false;
            }
            *tMin = t;
        }
    } else if ( pos < bounds[MIN_Z] || pos > bounds[MAX_Z] ) {
        return false;
    }

    // If *tmin == mindist, then there was no "near"
    // intersection farther than EPSILON away.
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

int
boundsIntersect(Ray *ray, float *bounds, float minimumDistance, float *maximumDistance) {
    float tMin;
    float tMax;
    int hit;

    tMin = minimumDistance;
    tMax = *maximumDistance;
    hit = boundsIntersectingSegment(ray, bounds, &tMin, &tMax);
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
int
boundsBehindPlane(const float *bounds, Vector3D *norm, float d) {
    Vector3D P;

    if ( norm->x > 0.0f ) {
        P.x = bounds[MAX_X];
    } else {
        P.x = bounds[MIN_X];
    }

    if ( norm->y > 0.0f ) {
        P.y = bounds[MAX_Y];
    } else {
        P.y = bounds[MIN_Y];
    }

    if ( norm->z > 0.0f ) {
        P.z = bounds[MAX_Z];
    } else {
        P.z = bounds[MIN_Z];
    }

    return VECTORDOTPRODUCT(*norm, P) + d <= 0.;
}

/**
Computes boundingbox after transforming bbx with xf. Result is filled
in transbbx and a pointer to transbbx returned
*/
float *
boundsTransform(float *bbx, Matrix4x4 *xf, float *transbbx) {
    Vector3D v[8];
    int i;
    float d;

    VECTORSET(v[0], bbx[MIN_X], bbx[MIN_Y], bbx[MIN_Z]);
    VECTORSET(v[1], bbx[MAX_X], bbx[MIN_Y], bbx[MIN_Z]);
    VECTORSET(v[2], bbx[MIN_X], bbx[MAX_Y], bbx[MIN_Z]);
    VECTORSET(v[3], bbx[MAX_X], bbx[MAX_Y], bbx[MIN_Z]);
    VECTORSET(v[4], bbx[MIN_X], bbx[MIN_Y], bbx[MAX_Z]);
    VECTORSET(v[5], bbx[MAX_X], bbx[MIN_Y], bbx[MAX_Z]);
    VECTORSET(v[6], bbx[MIN_X], bbx[MAX_Y], bbx[MAX_Z]);
    VECTORSET(v[7], bbx[MAX_X], bbx[MAX_Y], bbx[MAX_Z]);

    boundsInit(transbbx);
    for ( i = 0; i < 8; i++ ) {
        transformPoint3D(*xf, v[i], v[i]);
        boundsEnlargePoint(transbbx, &v[i]);
    }

    d = (transbbx[MAX_X] - transbbx[MIN_X]) * (float)EPSILON;
    transbbx[MIN_X] -= d;
    transbbx[MAX_X] += d;
    d = (transbbx[MAX_Y] - transbbx[MIN_Y]) * (float)EPSILON;
    transbbx[MIN_Y] -= d;
    transbbx[MAX_Y] += d;
    d = (transbbx[MAX_Z] - transbbx[MIN_Z]) * (float)EPSILON;
    transbbx[MIN_Z] -= d;
    transbbx[MAX_Z] += d;

    return transbbx;
}
