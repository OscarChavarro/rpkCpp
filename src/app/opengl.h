#ifndef __OPENGL__
#define __OPENGL__

#include "raycasting/common/Raytracer.h"

extern Raytracer *GLOBAL_raytracer_activeRaytracer;

extern int openGlRenderInitialized();
extern void openGlSaveScreen(char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);
extern void openGlRenderCameras();
extern void openGlRenderSetLineWidth(float width);
extern void openGlRenderBackground(Camera *camera);

#endif
