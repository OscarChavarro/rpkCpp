#ifndef __MAIN_MODEL__
#define __MAIN_MODEL__

#include "java/util/ArrayList.h"
#include "raycasting/common/Raytracer.h"

#define STRING_SIZE 1000

extern java::ArrayList<Patch *> *GLOBAL_scenePatches;
extern PatchSet *globalAppScenePatches;

extern Raytracer *globalRayTracingMethods[];

extern char globalRaytracingMethodsString[STRING_SIZE];
extern char *globalCurrentDirectory;
extern int globalYes;
extern int globalNo;
extern int globalImageOutputWidth;
extern int globalImageOutputHeight;

#endif
