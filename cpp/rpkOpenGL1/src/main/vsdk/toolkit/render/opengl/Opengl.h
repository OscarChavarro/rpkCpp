#ifndef __OPENGL__
#define __OPENGL__

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
    static void openGlRenderPatchFlat(const Patch *patch, const ColorRgb &color, const RendererConfiguration *renderOptions);
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
        const RpkGeometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatch,
        const RendererConfiguration *renderOptions);
    static void
    openGlRenderOctreeLeaf(
        const Camera *camera,
        const RpkGeometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RendererConfiguration *renderOptions);
    static bool openGlViewCullBounds(const Camera *camera, const RpkAxisAlignedBoundingBox *bounds);
    static double openGlBoundsDistance2(Vector3Dd p, const RpkAxisAlignedBoundingBox *boundingBox);
    static void
    openGlRenderOctreeNonLeaf(
        Camera *camera,
        const RpkGeometry *geometry,
        const OpenGlRenderTraversalCallback &renderPatchCallback,
        const RendererConfiguration *renderOptions);
    static void openGlRenderSetLineWidth(double width);
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
    static Vector3Dd sceneRotationPivot(const Scene *scene);
    static void viewportAxesInWorld(const Scene *scene, Vector3Dd *axisU, Vector3Dd *axisV);

  public:
    static void openGlRenderLine(Vector3Dd *x, Vector3Dd *y);
    static void openGlRenderSetColor(const ColorRgb *rgb, const RendererConfiguration *renderOptions);
    static void openGlRenderPatchOutline(const Patch *patch);
    static void openGlRenderPolygonFlat(int numberOfVertices, Vector3Dd *vertices);
    static void
    openGlRenderPolygonGouraud(
        int numberOfVertices,
        Vector3Dd *vertices,
        const ColorRgb *verticesColors,
        const RendererConfiguration *renderOptions);
    static void openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions, const FaceColorTable *faceColors);
    static void openGlRenderClearWindow(const Camera *camera);
    static void openGlRenderSetCamera(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries);
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
