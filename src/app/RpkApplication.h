#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "scene/Scene.h"
#include "io/context/ParseSession.h"
#include "raycasting/common/RayTracer.h"
#include "tonemap/ToneMappingContext.h"

class RayMatterState;
class BidirectionalPathTracingState;
class StochasticRayTracingState;
class StochasticRelaxation;
class ElementHierarchyState;
class StochasticRadiosityBasisState;
class PhotonMapState;
class PhotonMapConfig;
class LightList;
class OptionsType;

class RpkApplication {
  private:
    static constexpr bool DEFAULT_MONOCHROME = false;
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    ParseSession *mgfContext;
    RadianceMethod *selectedRadianceMethod;
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
