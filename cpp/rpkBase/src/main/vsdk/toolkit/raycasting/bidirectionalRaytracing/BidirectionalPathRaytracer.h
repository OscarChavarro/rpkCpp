#ifndef __BIDIRECTIONAL_PATH_RAYTRACER__
#define __BIDIRECTIONAL_PATH_RAYTRACER__

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

    static ColorRgb
    bpCalcPixel(
        Camera *camera,
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneSceneQueryWorkspace,
        Background *sceneBackground,
        int nx,
        int ny,
        BidirectionalPathTracingConfiguration *config);

    static bool spikeCheck(const ColorRgb &color);

    static void
    addWithSpikeCheck(
        BidirectionalPathTracingConfiguration *config,
        const BiPath *path,
        int nx,
        int ny,
        double pix_x,
        double pix_y,
        ColorRgb f,
        bool radSample = false);

    static void
    handlePathX0(
        Camera *camera,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static ColorRgb
    computeNeFluxEstimate(
        Camera *camera,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path,
        double *pPdf = nullptr,
        double *pWeight = nullptr,
        ColorRgb *fRad = nullptr);

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
        ImageOutputTarget *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions) const final;

    bool saveImage(ImageOutputTarget *imageOutputTarget) const final;
    void terminate() const;
};

#endif
#endif
