#ifndef __OPENGL__
#define __OPENGL__

#include "common/RenderOptions.h"
#include "skin/Patch.h"
#include "scene/RadianceMethod.h"
#include "scene/Camera.h"
#include "scene/Scene.h"

typedef void (*OpenGlRenderPatchCallback)(const Patch *, const Camera *, const RenderOptions *);
typedef void (*OpenGlRenderPatchCallbackWithData)(const Patch *, const Camera *, const RenderOptions *, void *);

extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(const ColorRgb *rgb);
extern void openGlRenderPatchOutline(const Patch *patch);
extern void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
extern void openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, const ColorRgb *verticesColors);
extern void softRenderPixels(int width, int height, const ColorRgb *rgb);
extern void openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
extern void openGlRenderClearWindow(const Camera *camera);
extern void openGlRenderSetCamera(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);

extern void
openGlRenderWorldOctree(
    const Scene *scene,
    OpenGlRenderPatchCallback renderPatchCallback,
    const RenderOptions *renderOptions);

extern void
openGlRenderWorldOctreeWithData(
    const Scene *scene,
    OpenGlRenderPatchCallbackWithData renderPatchCallback,
    void *callbackData,
    const RenderOptions *renderOptions);

extern void
openGlRenderScene(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions);

#endif
