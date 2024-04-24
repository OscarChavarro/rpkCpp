#ifndef __OPENGL__
#define __OPENGL__

#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

extern void openGlRenderSetLineWidth(float width);
extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(ColorRgb *rgb);

extern void
openGlRenderWorldOctree(
    Scene *scene,
    void (*renderPatchCallback)(Patch *, Camera *, RenderOptions *),
    RenderOptions *renderOptions);

extern void
openGlRenderScene(
    Scene *scene,
    int (*reDisplayCallback)(),
    RadianceMethod *context,
    RenderOptions *renderOptions);

extern void openGlMesaRenderCreateOffscreenWindow(Camera *camera, int width, int height);
extern void openGlRenderPatchOutline(Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, ColorRgb *verticesColors);
extern void openGlRenderPixels(Camera *camera, int x, int y, int width, int height, ColorRgb *rgb);
extern void openGlRenderPatch(Patch *patch, Camera *camera, RenderOptions *renderOptions);
extern void openGlRenderNewDisplayList(Geometry *clusteredWorldGeometry, RenderOptions *renderOptions);

#endif
