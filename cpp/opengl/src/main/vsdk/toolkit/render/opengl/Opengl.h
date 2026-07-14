#ifndef OPENGL__
#define OPENGL__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/render/opengl/OpenGLCallbacks.h"
#include "vsdk/toolkit/render/opengl/OpenGlRenderTraversalCallback.h"
#include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"

class Opengl {
  private:
    static const ToneMappingContext *activeToneMapOptions;
    static void openGlRenderPatchFlat(const Patch *patch, const RendererConfiguration *renderOptions);
    static void openGlRenderPatchSmooth(const Patch *patch, const RendererConfiguration *renderOptions);
    static void
    openGlInvokeRenderPatch(
        const OpenGlRenderTraversalCallback &renderPatch,
        const Patch *patch,
        const Camera *camera,
        const RendererConfiguration *renderOptions);
    static void
    openGlReallyRenderOctreeLeaf(
        const Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatch,
        const RendererConfiguration *renderOptions);
    static void
    openGlRenderOctreeLeaf(
        const Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RendererConfiguration *renderOptions);
    static bool openGlViewCullBounds(const Camera *camera, const AxisAlignedBoundingBox *bounds);
    static float openGlBoundsDistance2(Vector3D p, const AxisAlignedBoundingBox *boundingBox);
    static void
    openGlRenderOctreeNonLeaf(
        Camera *camera,
        const Geometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RendererConfiguration *renderOptions);
    static void openGlRenderSetLineWidth(float width);
    static void
    openGlReallyRender(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions,
        const GlutDebugState *debugState);
    static void
    openGlRenderRadiance(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions,
        const GlutDebugState *debugState);
    static Vector3D sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV);

  public:
    static void openGlRenderLine(Vector3D *x, Vector3D *y);
    static void openGlRenderSetColor(const ColorRgb *rgb, const RendererConfiguration *renderOptions);
    static void openGlRenderSetColor(const ColorRgbMutable *rgb, const RendererConfiguration *renderOptions);
    static void openGlRenderPatchOutline(const Patch *patch);
    static void openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices);
    static void
    openGlRenderPolygonGouraud(
        int numberOfVertices,
        Vector3D *vertices,
        const ColorRgbMutable *verticesColors,
        const RendererConfiguration *renderOptions);
    static void openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions);
    static void openGlRenderClearWindow(const Camera *camera);
    static void openGlRenderSetCamera(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries);
    static void openGlApplyDebugSceneRotation(const Scene *scene, const GlutDebugState *debugState);

    static void
    openGlRenderWorldOctree(
        const Scene *scene,
        OpenGlRenderPatchCallback renderPatchCallback,
        const RendererConfiguration *renderOptions);

    static void
    openGlRenderScene(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions,
        const GlutDebugState *debugState = nullptr);
};

#endif
