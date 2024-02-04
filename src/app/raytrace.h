#ifndef RPK_RAYTRACE_H
#define RPK_RAYTRACE_H

#include "raycasting/common/Raytracer.h"

extern void mainRayTracingDefaults();
extern void mainParseRayTracingOptions(int *argc, char **argv);
extern void mainSetRayTracingMethod(Raytracer *newMethod);

#endif
