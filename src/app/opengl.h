#ifndef __OPENGL__
#define __OPENGL__

#include "raycasting/common/Raytracer.h"

extern Raytracer *GLOBAL_raytracer_activeRaytracer;

extern int renderRayTraced(Raytracer *activeRayTracer);
extern int renderInitialized();
extern void saveScreen(char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);
extern void renderCameras();
extern void renderSetLineWidth(float width);
extern void renderBackground(Camera *camera);

#endif
