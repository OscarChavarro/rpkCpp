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
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config) const;

    void
    doBptDensityEstimation(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config) const;

    static ColorRgb
    bpCalcPixel(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
        Background *sceneBackground,
        int nx,
        int ny,
        void *data);

    static bool spikeCheck(ColorRgb color);

    static void
    addWithSpikeCheck(
        BidirectionalPathTracingConfiguration *config,
        const BiPath *path,
        int nx,
        int ny,
        float pix_x,
        float pix_y,
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
        float *pPdf = nullptr,
        float *pWeight = nullptr,
        ColorRgb *fRad = nullptr);

    static void
    handlePathXx(
        Camera *camera,
        VoxelGrid *sceneWorldVoxelGrid,
        Background *sceneBackground,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static void
    handlePath1X(
        Camera *camera,
        const VoxelGrid *sceneVoxelGrid,
        BidirectionalPathTracingConfiguration *config,
        BiPath *path);

    static void
    bpCombinePaths(
        Camera *camera,
        VoxelGrid *sceneVoxelGrid,
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
