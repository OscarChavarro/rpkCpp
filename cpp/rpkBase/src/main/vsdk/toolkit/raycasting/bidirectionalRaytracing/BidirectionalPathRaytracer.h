#ifndef BI_DIRECTIONAL_PATH__
#define BI_DIRECTIONAL_PATH__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"

class BidirectionalPathRaytracer final : public RayTracer {
  private:
    static constexpr int STRINGS_SIZE = 300;
    static char name[];
    BidirectionalPathTracingState &bidirectionalPathState;
    LightList *&lightList;

    void
    doBptAndSubsequentImages(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config) const;

    void
    doBptDensityEstimation(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config) const;

    static ColorRgbMutable
    bpCalcPixel(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        int nx,
        int ny,
        BidirectionalPathTracingConfiguration *config);

    static bool spikeCheck(ColorRgbMutable color);

    static void
    addWithSpikeCheck(
        BidirectionalPathTracingConfiguration *config,
        const BiPath *path,
        int nx,
        int ny,
        float pix_x,
        float pix_y,
        ColorRgbMutable f,
        bool radSample = false);

    static void
    handlePathX0(
        Camera *camera,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static ColorRgbMutable
    computeNeFluxEstimate(
        Camera *camera,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path,
        float *pPdf = nullptr,
        float *pWeight = nullptr,
        ColorRgbMutable *fRad = nullptr);

    static void
    handlePathXx(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static void
    handlePath1X(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static void
    bpCombinePaths(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config);

  public:
    BidirectionalPathRaytracer(
        BidirectionalPathTracingState &inBidirectionalPathState,
        LightList *&inLightList);
    ~BidirectionalPathRaytracer() final;

    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;

    void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const;
};

#endif
#endif
