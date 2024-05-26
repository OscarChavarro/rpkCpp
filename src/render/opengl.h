#ifndef __OPENGL__
#define __OPENGL__

#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

#include "raycasting/common/Raytracer.h"

extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(const ColorRgb *rgb);

extern void
openGlRenderWorldOctree(
    const Scene *scene,
    void (*renderPatchCallback)(Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions);

extern void openGlMesaRenderCreateOffscreenWindow(const Camera *camera, int width, int height);
extern void openGlRenderPatchOutline(const Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, const ColorRgb *verticesColors);
extern void openGlRenderPixels(const Camera *camera, int x, int y, int width, int height, const ColorRgb *rgb);
extern void openGlRenderPatchCallBack(Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
extern void openGlRenderClearWindow(const Camera *camera);

#ifdef RAYTRACING_ENABLED
extern void openGlRenderNewDisplayList(Geometry *clusteredWorldGeometry, const RenderOptions *renderOptions);
#endif

#endif
