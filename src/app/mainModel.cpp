#include "raycasting/simple/RayCaster.h"
#include "raycasting/simple/RayMatter.h"
#include "raycasting/raytracing/BidirectionalPathRaytracer.h"
#include "raycasting/stochasticRaytracing/StochasticRaytracer.h"
#include "app/mainModel.h"

// The list of all patches in the current scene. Automatically derived from 'GLOBAL_scene_world' when loading a scene
PatchSet *globalAppScenePatches = nullptr;
#define STRING_SIZE 1000

Raytracer *globalRayTracingMethods[] = {
    &GLOBAL_raytracing_stochasticMethod,
    &GLOBAL_raytracing_biDirectionalPathMethod,
    &GLOBAL_rayCasting_RayCasting,
    &GLOBAL_rayCasting_RayMatting,
    nullptr
};

char globalRaytracingMethodsString[STRING_SIZE];
char *globalCurrentDirectory;
int globalYes = 1;
int globalNo = 0;
int globalImageOutputWidth = 0;
int globalImageOutputHeight = 0;
