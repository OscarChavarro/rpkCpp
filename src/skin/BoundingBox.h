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
    outOfBounds(Vector3D *p) const {
        return p->x < coordinates[MIN_X] || p->x > coordinates[MAX_X] ||
               p->y < coordinates[MIN_Y] || p->y > coordinates[MAX_Y] ||
               p->z < coordinates[MIN_Z] || p->z > coordinates[MAX_Z];
    }
};

/**
true if the two given bounding boxes are disjunct
*/
inline bool
disjunctBounds(const float *b1, const float *b2) {
    return
        (b1[MIN_X] > b2[MAX_X]) || (b2[MIN_X] > b1[MAX_X]) ||
        (b1[MIN_Y] > b2[MAX_Y]) || (b2[MIN_Y] > b1[MAX_Y]) ||
        (b1[MIN_Z] > b2[MAX_Z]) || (b2[MIN_Z] > b1[MAX_Z]);
}

extern float *boundsCreate();
extern float *boundsCopy(const float *from, float *to);
extern float *boundsInit(float *bounds);
extern float *boundsEnlarge(float *output, float *input);
extern float *boundsEnlargePoint(float *bounds, Vector3D *point);
extern int boundsIntersect(Ray *ray, float *bounds, float minimumDistance, float *maximumDistance);
extern int boundsBehindPlane(const float *bounds, Vector3D *norm, float d);
extern float *boundsTransform(float *bbx, Matrix4x4 *xf, float *transBoundingBox);
extern int boundsIntersectingSegment(Ray *ray, const float *bounds, float *tMin, float *tMax);

#endif
