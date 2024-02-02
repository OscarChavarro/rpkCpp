#ifndef __OPENGL__
#define __OPENGL__

#include "skin/Patch.h"
#include "shared/Camera.h"

extern int openGlRenderInitialized();
extern void openGlSaveScreen(char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches);
extern void openGlRenderCameras();
extern void openGlRenderSetLineWidth(float width);
extern void openGlRenderBackground(Camera *camera);
extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(RGB *rgb);
extern void openGlRenderWorldOctree(void (*render_patch)(Patch *));
extern void openGlRenderScene(java::ArrayList<Patch *> *scenePatches, int (*reDisplayCallback)());
extern void openGlMesaRenderCreateOffscreenWindow(int width, int height);
extern void openGlRenderBeginTriangleStrip();
extern void openGlRenderNextTrianglePoint(Vector3D *point, RGB *col);
extern void openGlRenderEndTriangleStrip();
extern void openGlRenderPatchOutline(Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, RGB *verticesColors);
extern void openGlRenderPixels(int x, int y, int width, int height, RGB *rgb);
extern void openGlRenderPatch(Patch *patch);
extern void openGlRenderNewDisplayList();

#endif
