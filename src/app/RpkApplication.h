#ifndef __RPK_APPLICATION__
#define __RPK_APPLICATION__

#include "io/mgf/MgfContext.h"
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

    void mainInitApplication();
    void mainParseOptions(int *argc, char **argv);
    void mainCreateOffscreenCanvasWindow();
    void executeRendering();
    void freeMemory();

  public:
    RpkApplication();
    ~RpkApplication();
    int entryPoint(int argc, char *argv[]);
};

#endif
