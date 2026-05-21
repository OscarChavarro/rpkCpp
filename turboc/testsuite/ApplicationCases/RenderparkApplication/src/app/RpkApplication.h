#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "vsdk/material/RendererConfiguration.h"
#include "vsdk/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/LightList.h"
#include "vsdk/raycasting/simple/RayMatterState.h"
#include "vsdk/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/scene/Scene.h"
#include "vsdk/io/context/ParseRuntimeContext.h"
#include "vsdk/raycasting/common/RayTracer.h"
#include "vsdk/tonemap/ToneMap.h"

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
