#ifndef __RADIANCE__
#define __RADIANCE__

#include "skin/RadianceMethod.h"

extern RadianceMethod *GLOBAL_radiance_selectedRadianceMethod;

extern void radianceDefaults(java::ArrayList<Patch *> *scenePatches, RadianceMethod *context);
extern void parseRadianceOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);

#endif
