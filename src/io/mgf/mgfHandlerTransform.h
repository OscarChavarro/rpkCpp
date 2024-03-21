#ifndef __MGF_HANDLER_TRANSFORM__
#define __MGF_HANDLER_TRANSFORM__

#include "skin/RadianceMethod.h"
#include "io/mgf/Vector3Dd.h"

/**
The transformation handler should do most of the work that needs
doing. Just globalPass it any xf entities, then use the associated
functions to transform and translate positions, transform vectors
(without translation), rotate vectors (without scaling) and scale
values appropriately.

The routines mgfTransformPoint, mgfTransformVector and xf_rotvect take two
3-D vectors (which may be identical), transforms the second and
puts the result into the first.
*/

extern int handleTransformationEntity(int ac, char **av, RadianceMethod * /*context*/);
extern void mgfTransformPoint(VECTOR3Dd v1, VECTOR3Dd v2); // Transform point
extern void mgfTransformVector(VECTOR3Dd v1, VECTOR3Dd v2); // Transform vector

#endif
