#ifndef __OPENGL__
#define __OPENGL__

#include "common/RenderOptions.h"
#include "render/opengl/visualDebugTools/GlutDebugState.h"
#include "skin/Patch.h"
#include "scene/Camera.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"

class OpenGlRenderTraversalCallback;

typedef void (*OpenGlRenderPatchCallback)(const Patch *, const Camera *, const RenderOptions *);
typedef void (*OpenGlRenderPatchCallbackWithData)(const Patch *, const Camera *, const RenderOptions *, void *);

class Opengl {
  private:
    static const ToneMappingContext *activeToneMapOptions;
    static void openGlRenderPatchFlat(const Patch *patch, const RenderOptions *renderOptions);
    static void openGlRenderPatchSmooth(const Patch *patch, const RenderOptions *renderOptions);
    static void
    openGlInvokeRenderPatch(
        const OpenGlRenderTraversalCallback &renderPatch,
        const Patch *patch,
        const Camera *camera,
        const RenderOptions *renderOptions);
    static void
    openGlReallyRenderOctreeLeaf(
        const Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatch,
        const RenderOptions *renderOptions);
    static void
    openGlRenderOctreeLeaf(
        const Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RenderOptions *renderOptions);
    static bool openGlViewCullBounds(const Camera *camera, const BoundingBox *bounds);
    static float openGlBoundsDistance2(Vector3D p, const BoundingBox *boundingBox);
    static void
    openGlRenderOctreeNonLeaf(
        Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RenderOptions *renderOptions);
    static void openGlRenderSetLineWidth(float width);
    static void
    openGlReallyRender(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions,
        const GlutDebugState *debugState);
    static void
    openGlRenderRadiance(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions,
        const GlutDebugState *debugState);
    static Vector3D sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV);

  public:
    static void openGlRenderLine(Vector3D *x, Vector3D *y);
    static void openGlRenderSetColor(const ColorRgb *rgb, const RenderOptions *renderOptions);
    static void openGlRenderPatchOutline(const Patch *patch);
    static void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
    static void
    openGlRenderPolygonGouraud(
        int numberOfVertices,
        Vector3D *vertices,
        const ColorRgb *verticesColors,
        const RenderOptions *renderOptions);
    static void openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
    static void openGlRenderClearWindow(const Camera *camera);
    static void openGlRenderSetCamera(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);
    static void openGlApplyDebugSceneRotation(const Scene *scene, const GlutDebugState *debugState);

    static void
    openGlRenderWorldOctree(
        const Scene *scene,
        OpenGlRenderPatchCallback renderPatchCallback,
        const RenderOptions *renderOptions);

    static void
    openGlRenderWorldOctreeWithData(
        const Scene *scene,
        OpenGlRenderPatchCallbackWithData renderPatchCallback,
        void *callbackData,
        const RenderOptions *renderOptions);

    static void
    openGlRenderScene(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const ToneMappingContext *toneMapOptions,
        const RenderOptions *renderOptions,
        const GlutDebugState *debugState = nullptr);
};

#endif
