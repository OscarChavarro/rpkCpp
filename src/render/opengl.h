#ifndef __OPENGL__
#define __OPENGL__

#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(const ColorRgb *rgb);

extern void
openGlRenderWorldOctree(
    const Scene *scene,
    void (*renderPatchCallback)(const Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions);

extern void openGlRenderPatchOutline(const Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, const ColorRgb *verticesColors);
extern void softRenderPixels(int width, int height, const ColorRgb *rgb);
extern void openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
extern void openGlRenderClearWindow(const Camera *camera);

#endif
