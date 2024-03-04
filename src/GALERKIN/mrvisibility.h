/**
Multi-Resolution Visibility ala Sillion & Drettakis, "Multi-Resolution
Control of Visibility Error", SIGGRAPH '95 p145
*/

#ifndef __MR_VISIBILITY__
#define __MR_VISIBILITY__

#include "common/Ray.h"

/**
Following routines return a floating point number in the range [0..1].
0 indicates that there is
full occlusion, 1 that there is full visibility and a number in between
that there is visibility with at least one occluder with feature size
smaller than the specified minimum feature size. (Such occluders have been
replaced by a box containing an isotropic participating medium with
suitable extinction properties.) rcvdist is the distance from the origin of
the ray (assumed to be on the source) to the receiver surface. srcsize
is the diameter of the source surface. min feature size is the minimal
diameter of a feature (umbra or whole lit region on the receiver)
that one is interested in. Approximate visibility computations are allowed
for occluders that cast features with diameter smaller than
min feature size. If there is a "hard" occlusion, the first patch tested that
lead to this conclusion is added to the shadow cache
*/
extern double
geomListMultiResolutionVisibility(java::ArrayList<Geometry *> *geometryOccluderList, Ray *ray, float rcvdist, float srcSize, float minimumFeatureSize);


/* Equivalent blocker size determination: first call BlockerInit(),
 * then call GeomBlcokerSize() of GeomBlockserSizeInDirection() for the
 * geoms for which you like to compute the equivalent blocker size, and
 * finally terminate with BlockerTerminate(). */

extern void blockerInit();

extern void blockerTerminate();

#endif
