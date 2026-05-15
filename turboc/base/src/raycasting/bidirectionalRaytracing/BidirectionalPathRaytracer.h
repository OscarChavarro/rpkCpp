#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __BI_DIRECTIONAL_PATH__
#define __BI_DIRECTIONAL_PATH__

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingConfiguration.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"

class BidirectionalPathRaytracer: public RayTracer{ private:
    #define STRINGS_SIZE 300
    static char name[];
    BidirectionalPathTracingState &bidirectionalPathState;
    LightList *&lightList;

    void
    doBptAndSubsequentImages( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, BidirPathTraceConfig *config) const;

    void
    doBptDensityEstimation( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, BidirPathTraceConfig *config) const;

    static ColorRgb
    bpCalcPixel( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, int nx, int ny, void *data);

    static bool spikeCheck(ColorRgb color);

    static void
    addWithSpikeCheck( BidirPathTraceConfig *config, const BiPath *path, int nx, int ny, float pix_x, float pix_y, ColorRgb f, bool radSample = false);

    static void
    handlePathX0( Camera *camera, Background *sceneBackground, BidirPathTraceConfig *config, BiPath *path);

    static ColorRgb
    computeNeFluxEstimate( Camera *camera, BidirPathTraceConfig *config, BiPath *path, float *pPdf = NULL, float *pWeight = NULL, ColorRgb *fRad = NULL);

    static void
    handlePathXx( Camera *camera, VoxelGrid *sceneWorldVoxelGrid, Background *sceneBackground, BidirPathTraceConfig *config, BiPath *path);

    static void
    handlePath1X( Camera *camera, const VoxelGrid *sceneVoxelGrid, BidirPathTraceConfig *config, BiPath *path);

    static void
    bpCombinePaths( Camera *camera, VoxelGrid *sceneVoxelGrid, Background *sceneBackground, BidirPathTraceConfig *config);

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
