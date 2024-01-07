#ifndef __BOUNDS__
#define __BOUNDS__

#include "common/linealAlgebra/vectorMacros.h"
#include "common/linealAlgebra/Matrix4x4.h"
#include "common/Ray.h"

/**
A bounding box is represented as a n array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X ... below
*/
typedef float BOUNDINGBOX[6];

/**
 * the following defines must obay the following rules:
 * 1) DIR_X = (MIN_X) and DIR_X = (MAX_X) and same for DIR_Y, DIR_Z 
 * 2) (MIN_X + 3) = MAX_X and (MAX_X + 3) = MIN_X and same for MIN_Y, ...
 * 3) MIN_X + 1 = MIN_Y, MIN_Y + 1 = MIN_Z and MAX_X + 1 = MAX_Y, MAX_Y+1 = MAX_Z 
 */
#define MIN_X 0
#define MIN_Y 1
#define MIN_Z 2
#define MAX_X 3
#define MAX_Y 4
#define MAX_Z 5

inline bool
OutOfBounds(Vector3D *p, float * bounds) {
    return p->x < bounds[MIN_X] || p->x > bounds[MAX_X] ||
           p->y < bounds[MIN_Y] || p->y > bounds[MAX_Y] ||
           p->z < bounds[MIN_Z] || p->z > bounds[MAX_Z];
}

/**
true if the two given boundingboxes are disjunct
*/
inline bool
DisjunctBounds(float *b1, float *b2) {
    return
        (b1[MIN_X] > b2[MAX_X]) || (b2[MIN_X] > b1[MAX_X]) ||
        (b1[MIN_Y] > b2[MAX_Y]) || (b2[MIN_Y] > b1[MAX_Y]) ||
        (b1[MIN_Z] > b2[MAX_Z]) || (b2[MIN_Z] > b1[MAX_Z]);
}

extern float * boundsCreate();
extern float * boundsCopy(const float *from, float *to);
extern void boundsDestroy(float *bounds);
extern float * boundsInit(float *bounds);
extern float * boundsEnlarge(float *bounds, float *extra);
extern float * boundsEnlargePoint(float *bounds, Vector3D *point);
extern int boundsIntersect(Ray *ray, float *bounds, float mindist, float *maxdist);
extern int boundsBehindPlane(float *bounds, Vector3D *norm, float d);
extern float *boundsTransform(float *bbx, Matrix4x4 *xf, float *transbbx);
extern int boundsIntersectingSegment(Ray *ray, float *bounds, float *tmin, float *tmax);

#endif
