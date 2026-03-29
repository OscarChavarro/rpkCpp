#ifndef __MGF_HANDLER_TRANSFORM__
#define __MGF_HANDLER_TRANSFORM__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/context/MgfParseSession.h"

/**
The transformation handler should do most of the work that needs
doing. Just globalPass it any xf entities, then use the associated
functions to transform and translate positions, transform vectors
(without translation), rotate vectors (without scaling) and scale
values appropriately.

The routines mgfTransformPoint and mgfTransformVector takes two
3-D vectors (which may be identical), transforms the second and
puts the result into the first.
*/

extern int handleTransformationEntity(int ac, const char **av, MgfParseSession * /*context*/);
extern void mgfTransformPoint(Vector3Dd *v1, const Vector3Dd *v2, const MgfParseSession *context); // Transform point
extern void mgfTransformVector(Vector3Dd *v1, const Vector3Dd *v2, const MgfParseSession *context); // Transform vector
extern void mgfTransformFreeMemory(MgfParseSession *context);

#endif
