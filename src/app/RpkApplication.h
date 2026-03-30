#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "scene/Scene.h"
#include "io/context/ParseSession.h"
#include "raycasting/common/RayTracer.h"
#include "tonemap/ToneMappingContext.h"

class RpkApplication {
  private:
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
    void mainParseOptions(int *argc, char **argv, char *rayTracerName, char *toneMapName);
    void mainCreateOffscreenCanvasWindow() const;
    void executeRendering(const char *rayTracerName);
    static void freeMemory(ParseSession *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();

    int entryPoint(int argc, char *argv[]);
};

#endif
