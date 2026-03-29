#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "scene/Scene.h"
#include "io/context/BaseContext.h"
#include "raycasting/common/RayTracer.h"

class RpkApplication {
  private:
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    BaseContext *mgfContext;
    RadianceMethod *selectedRadianceMethod;
    RenderOptions *renderOptions;
    RayTracer *rayTracer;

    static void selectToneMapByName(const char *name);
    static void mainInitApplication();
    void mainParseOptions(int *argc, char **argv, char *rayTracerName, char *toneMapName);
    void mainCreateOffscreenCanvasWindow() const;
    void executeRendering(const char *rayTracerName);
    static void freeMemory(BaseContext *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();

    int entryPoint(int argc, char *argv[]);
};

#endif
