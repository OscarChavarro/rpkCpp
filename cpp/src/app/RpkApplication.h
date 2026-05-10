#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "material/RendererConfiguration.h"
#include "scene/Scene.h"
#include "io/context/ParseRuntimeContext.h"
#include "raycasting/common/RayTracer.h"
#include "tonemap/ToneMap.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/photonMap/PhotonMapConfig.h"
    #include "raycasting/photonMap/PhotonMapState.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/bidirectionalRaytracing/LightList.h"
    #include "raycasting/simple/RayMatterState.h"
    #include "raycasting/stochasticRaytracing/Basismcrad.h"
    #include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#endif

class RpkApplication {
  private:
    static constexpr bool DEFAULT_MONOCHROME = false;
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    ParseRuntimeContext *mgfContext;
    RadianceMethod *selectedRadianceMethod;
    ToneMap *selectedToneMap;
    ToneMappingContext toneMapOptions;
    RendererConfiguration *renderOptions;
    RayTracer *rayTracer;
    bool glutDebugEnabled;

    void selectToneMapByName(const char *name);
    static bool isRaytracingDependentOption(const char *argument);
    static void failIfUnsupportedRaytracingOptionRequested(int argc, char **argv);
    static void mainInitApplication();
    void mainParseOptions(
            int *argc,
            char **argv,
            char *rayTracerName,
            char *toneMapName
#ifdef RAYTRACING_ENABLED
            ,
            StochasticRelaxation &stochasticRelaxationState,
            ElementHierarchyState &elementHierarchyState,
            StochasticRadiosityBasisState &stochasticRadiosityBasisState,
            PhotonMapState &photonMapState,
            PhotonMapConfig &photonMapConfig,
            RayMatterState &rayMatterState,
            BidirectionalPathTracingState &bidirectionalPathState,
            StochasticRayTracingState &stochasticRayTracingState
#endif
            );
    void mainCreateOffscreenCanvasWindow() const;
    void executeRendering(
        const char *rayTracerName
#ifdef RAYTRACING_ENABLED
        ,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList
#endif
        );
    static void freeMemory(ParseRuntimeContext *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();

    int entryPoint(int argc, char *argv[]);
};

#endif
