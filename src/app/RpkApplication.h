#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "io/mgf/MgfContext.h"
#include "raycasting/common/Raytracer.h"
#include "scene/Scene.h"

class RpkApplication {
  private:
    static Material defaultMaterial;
    int imageOutputWidth;
    int imageOutputHeight;
    Scene *scene;
    MgfContext *mgfContext;
    RadianceMethod *selectedRadianceMethod;
    RenderOptions *renderOptions;
    RayTracer *rayTracer;

    static void selectToneMapByName(char *name);
    void mainInitApplication();
    void mainParseOptions(int *argc, char **argv, char *rayTracerName, char *toneMapName);
    void mainCreateOffscreenCanvasWindow();
    void executeRendering(const char *rayTracerName);
    static void freeMemory(MgfContext *mgfContext);

  public:
    RpkApplication();
    ~RpkApplication();
    int entryPoint(int argc, char *argv[]);
};

#endif
