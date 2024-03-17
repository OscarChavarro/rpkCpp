#ifndef __RADIANCE__
#define __RADIANCE__

#include "skin/RadianceMethod.h"

extern void radianceDefaults(java::ArrayList<Patch *> *scenePatches, RadianceMethod *context);
extern void parseRadianceOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);

#endif
