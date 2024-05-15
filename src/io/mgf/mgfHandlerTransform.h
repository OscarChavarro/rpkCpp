#ifndef __MGF_HANDLER_TRANSFORM__
#define __MGF_HANDLER_TRANSFORM__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/mgf/MgfContext.h"

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

extern int handleTransformationEntity(int ac, const char **av, MgfContext * /*context*/);
extern void mgfTransformPoint(VECTOR3Dd v1, const VECTOR3Dd v2, MgfContext *context); // Transform point
extern void mgfTransformVector(VECTOR3Dd v1, const VECTOR3Dd v2, MgfContext *context); // Transform vector
extern void mgfTransformFreeMemory();

#endif
