#include <cstdlib>

#include "common/mymath.h"
#include "bounds.h"

#define SetIfLess(a, b) (a = ((a) < (b) ? (a) : (b)))
#define SetIfGreater(a, b) (a = ((a) > (b) ? (a) : (b)))

float *
BoundsCreate() {
    float *bounds;

    bounds = (float *)malloc(sizeof(BOUNDINGBOX));
    return bounds;
}

void
BoundsDestroy(float *bounds) {
    free(bounds);
}

float *
BoundsInit(float *bounds) {
    bounds[MIN_X] = bounds[MIN_Y] = bounds[MIN_Z] = HUGE;
    bounds[MAX_X] = bounds[MAX_Y] = bounds[MAX_Z] = -HUGE;
    return bounds;
}

/**
Enlarge getBoundingBox with extra
*/
float *
BoundsEnlarge(float *bounds, float *extra) {
    SetIfLess(bounds[MIN_X], extra[MIN_X]);
    SetIfLess(bounds[MIN_Y], extra[MIN_Y]);
    SetIfLess(bounds[MIN_Z], extra[MIN_Z]);
    SetIfGreater(bounds[MAX_X], extra[MAX_X]);
    SetIfGreater(bounds[MAX_Y], extra[MAX_Y]);
    SetIfGreater(bounds[MAX_Z], extra[MAX_Z]);
    return bounds;
}

float *
BoundsEnlargePoint(float *bounds, Vector3D *point) {
    SetIfLess(bounds[MIN_X], point->x);
    SetIfLess(bounds[MIN_Y], point->y);
    SetIfLess(bounds[MIN_Z], point->z);
    SetIfGreater(bounds[MAX_X], point->x);
    SetIfGreater(bounds[MAX_Y], point->y);
    SetIfGreater(bounds[MAX_Z], point->z);
    return bounds;
}

float *
BoundsCopy(const float *from, float *to) {
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
call BoundsIntersect() only if it is not.

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
BoundsIntersectingSegment(Ray *ray, float *bounds, float *tmin, float *tmax) {
    float t, mindist, maxdist;
    float dir, pos;

    mindist = *tmin;
    maxdist = *tmax;

    dir = ray->dir.x;
    pos = ray->pos.x;

    if ( dir < 0 ) {
        t = (bounds[MIN_X] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MAX_X] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( dir > 0. ) {
        t = (bounds[MAX_X] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MIN_X] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( pos < bounds[MIN_X] || pos > bounds[MAX_X] ) {
        return false;
    }

    dir = ray->dir.y;
    pos = ray->pos.y;

    if ( dir < 0 ) {
        t = (bounds[MIN_Y] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MAX_Y] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( dir > 0. ) {
        t = (bounds[MAX_Y] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MIN_Y] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( pos < bounds[MIN_Y] || pos > bounds[MAX_Y] ) {
        return false;
    }

    dir = ray->dir.z;
    pos = ray->pos.z;

    if ( dir < 0 ) {
        t = (bounds[MIN_Z] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MAX_Z] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( dir > 0. ) {
        t = (bounds[MAX_Z] - pos) / dir;
        if ( t < *tmin ) {
            return false;
        }
        if ( t <= *tmax ) {
            *tmax = t;
        }
        t = (bounds[MIN_Z] - pos) / dir;
        if ( t >= *tmin ) {
            if ( t > *tmax * (1. + EPSILON)) {
                return false;
            }
            *tmin = t;
        }
    } else if ( pos < bounds[MIN_Z] || pos > bounds[MAX_Z] ) {
        return false;
    }

    //If *tmin == mindist, then there was no "near"
    //intersection farther than EPSILON away.
    if ( *tmin == mindist ) {
        if ( *tmax < maxdist ) {
            return true;
        }
    } else {
        if ( *tmin < maxdist ) {
            return true;
        }
    }
    return false; // hit, but not closer than maxdist
}

int
BoundsIntersect(Ray *ray, float *bounds, float mindist, float *maxdist) {
    float tmin, tmax;
    int hit;

    tmin = mindist;
    tmax = *maxdist;
    hit = BoundsIntersectingSegment(ray, bounds, &tmin, &tmax);
    if ( hit ) {
        // reduce maxdist
        if ( tmin == mindist ) {
            if ( tmax < *maxdist ) {
                *maxdist = tmax;
            }
        } else {
            if ( tmin < *maxdist ) {
                *maxdist = tmin;
            }
        }
    }
    return hit;
}

/**
Returns true if the boundingbox is behind the plane defined by norm and d
see F. Tampieri, Fast Vertex Radiosity Update, Graphics Gems II, p 303
*/
int
BoundsBehindPlane(float *bounds, Vector3D *norm, float d) {
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
BoundsTransform(float *bbx, Matrix4x4 *xf, float *transbbx) {
    Vector3D v[8];
    int i;
    double d;

    VECTORSET(v[0], bbx[MIN_X], bbx[MIN_Y], bbx[MIN_Z]);
    VECTORSET(v[1], bbx[MAX_X], bbx[MIN_Y], bbx[MIN_Z]);
    VECTORSET(v[2], bbx[MIN_X], bbx[MAX_Y], bbx[MIN_Z]);
    VECTORSET(v[3], bbx[MAX_X], bbx[MAX_Y], bbx[MIN_Z]);
    VECTORSET(v[4], bbx[MIN_X], bbx[MIN_Y], bbx[MAX_Z]);
    VECTORSET(v[5], bbx[MAX_X], bbx[MIN_Y], bbx[MAX_Z]);
    VECTORSET(v[6], bbx[MIN_X], bbx[MAX_Y], bbx[MAX_Z]);
    VECTORSET(v[7], bbx[MAX_X], bbx[MAX_Y], bbx[MAX_Z]);

    BoundsInit(transbbx);
    for ( i = 0; i < 8; i++ ) {
        TRANSFORM_POINT_3D(*xf, v[i], v[i]);
        BoundsEnlargePoint(transbbx, &v[i]);
    }

    d = (transbbx[MAX_X] - transbbx[MIN_X]) * EPSILON;
    transbbx[MIN_X] -= d;
    transbbx[MAX_X] += d;
    d = (transbbx[MAX_Y] - transbbx[MIN_Y]) * EPSILON;
    transbbx[MIN_Y] -= d;
    transbbx[MAX_Y] += d;
    d = (transbbx[MAX_Z] - transbbx[MIN_Z]) * EPSILON;
    transbbx[MIN_Z] -= d;
    transbbx[MAX_Z] += d;

    return transbbx;
}
