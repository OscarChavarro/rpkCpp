#ifndef __OPENGL__
#define __OPENGL__

#include "skin/Patch.h"
#include "scene/Camera.h"

extern int openGlRenderInitialized();
extern void openGlRenderSetLineWidth(float width);
extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(ColorRgb *rgb);

extern void
openGlRenderWorldOctree(
    Camera *camera,
    void (*renderPatchCallback)(Patch *, Camera *),
    Geometry *clusteredWorldGeometry);

extern void
openGlRenderScene(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *clusteredGeometryList,
    java::ArrayList<Geometry *> *sceneGeometries,
    Geometry *clusteredWorldGeometry,
    int (*reDisplayCallback)(),
    RadianceMethod *context);

extern void openGlMesaRenderCreateOffscreenWindow(int width, int height);
extern void openGlRenderPatchOutline(Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, ColorRgb *verticesColors);
extern void openGlRenderPixels(int x, int y, int width, int height, ColorRgb *rgb);
extern void openGlRenderPatch(Patch *patch, Camera *camera);
extern void openGlRenderNewDisplayList(Geometry *clusteredWorldGeometry);

#endif
