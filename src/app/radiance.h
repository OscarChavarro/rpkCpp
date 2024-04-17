#ifndef __RADIANCE__
#define __RADIANCE__

#include "scene/RadianceMethod.h"

extern void radianceDefaults(RadianceMethod *context, Geometry *clusteredWorldGeometry);
extern void parseRadianceOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);

#endif
