#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __BI_DIRECTIONAL_PATH__
#define __BI_DIRECTIONAL_PATH__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"

class BidirectionalPathRaytracer: public RayTracer{ private:
    #define STRINGS_SIZE 300
    static char name[];
    BidirectionalPathTracingState &bidirectionalPathState;
    LightList *&lightList;

    void
    doBptAndSubsequentImages( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, BidirPathTraceConfig *config) const;

    void
    doBptDensityEstimation( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, BidirPathTraceConfig *config) const;

  public:
    static ColorRgb
    bpCalcPixel( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, int nx, int ny, BidirPathTraceConfig *config);
  private:

    static bool spikeCheck(ColorRgb color);

    static void
    addWithSpikeCheck( BidirPathTraceConfig *config, const BiPath *path, int nx, int ny, float pix_x, float pix_y, ColorRgb f, bool radSample = false);

    static void
    handlePathX0( Camera *camera, Background *sceneBackground, BidirPathTraceConfig *config, BiPath *path);

    static ColorRgb
    computeNeFluxEstimate( Camera *camera, BidirPathTraceConfig *config, BiPath *path, float *pPdf = NULL, float *pWeight = NULL, ColorRgb *fRad = NULL);

    static void
    handlePathXx( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, Background *sceneBackground, BidirPathTraceConfig *config, BiPath *path);

    static void
    handlePath1X( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, BidirPathTraceConfig *config, BiPath *path);

    static void
    bpCombinePaths( Camera *camera, const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneSceneQueryWorkspace, Background *sceneBackground, BidirPathTraceConfig *config);

  public:
    BidirectionalPathRaytracer( BidirectionalPathTracingState &inBidirectionalPathState, LightList *&inLightList);
    ~BidirectionalPathRaytracer();

    void defaults();
    const char *getName() const;
    void initialize(const ArrayList<Patch *> *lightPatches) const;

    void
    execute( ImageOutputHandle *ip, Scene *scene, RadianceMethod *radianceMethod, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions) const;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const;
    void terminate() const;
};

#endif
#endif
