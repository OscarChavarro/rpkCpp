#ifndef __RADIANCE__
#define __RADIANCE__

#include "scene/RadianceMethod.h"

extern void radianceDefaults(RadianceMethod *context, Scene *scene);
extern void radianceParseOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);
extern void setRadianceMethod(RadianceMethod *newMethod, Scene *scene);

#endif
