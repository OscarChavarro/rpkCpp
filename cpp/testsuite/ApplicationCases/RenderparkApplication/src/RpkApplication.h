#ifndef RPK_APPLICATION__
#define RPK_APPLICATION__

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

#ifdef RAYTRACING_ENABLED
    #include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
    #include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
    #include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList.h"
    #include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"
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
