#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "material/RendererConfiguration.h"
#include "raycasting/photonMap/PhotonMapConfig.h"
#include "raycasting/photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "scene/Scene.h"
#include "io/context/ParseRuntimeContext.h"
#include "raycasting/common/RayTracer.h"
#include "tonemap/ToneMap.h"

class RpkApplication {
  private:
    #define DEFAULT_MONOCHROME false
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    ParseRuntimeContext *mgfContext;
    RadianceMethod *selectedRadianceMethod;
    ToneMap *selectedToneMap;
    ToneMappingContext toneMapOptions;
    RenderOptions *renderOptions;
    RayTracer *rayTracer;
    bool glutDebugEnabled;

    void selectToneMapByName(const char *name);
    static void mainInitApplication();
    void mainParseOptions(
            int *argc,
            char **argv,
            char *rayTracerName,
            char *toneMapName,
            StochasticRelaxation &stochasticRelaxationState,
            ElementHierarchyState &elementHierarchyState,
            StochasticRadiosityBasisState &stochasticRadiosityBasisState,
            PhotonMapState &photonMapState,
            PhotonMapConfig &photonMapConfig,
            RayMatterState &rayMatterState,
            BidirectionalPathTracingState &bidirectionalPathState,
            StochasticRayTracingState &stochasticRayTracingState);
    void mainCreateOffscreenCanvasWindow() const;
    void executeRendering(
        const char *rayTracerName,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList);
    static void freeMemory(ParseRuntimeContext *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();

    int entryPoint(int argc, char *argv[]);
};

#endif
