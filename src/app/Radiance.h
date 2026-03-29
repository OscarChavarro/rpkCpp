#ifndef __RADIANCE__
#define __RADIANCE__

#include "scene/RadianceMethod.h"

extern void radianceParseOptions(int *argc, char **argv, RadianceMethod **newRadianceMethod);
extern void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene);

#endif
