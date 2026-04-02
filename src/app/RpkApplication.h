#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "app/options/OptionsType.h"
#include "common/RenderOptions.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/bidirectionalRaytracing/LightList.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "scene/Scene.h"
#include "io/context/ParseSession.h"
#include "raycasting/common/RayTracer.h"
#include "tonemap/ToneMap.h"

class RpkApplication {
  private:
    static constexpr bool DEFAULT_MONOCHROME = false;
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    ParseSession *mgfContext;
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
            StochasticRayTracingState &stochasticRayTracingState,
            OptionsType &optionTypes);
    void mainCreateOffscreenCanvasWindow() const;
    void executeRendering(
        const char *rayTracerName,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        LightList *&lightList);
    static void freeMemory(ParseSession *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();

    int entryPoint(int argc, char *argv[]);
};

#endif
