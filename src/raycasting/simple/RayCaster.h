#ifndef __RAY_CASTER__
#define __RAY_CASTER__

#include "common/RenderOptions.h"
#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "render/ScreenBuffer.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
#endif

class RayCaster {
  private:
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;

    static void clipUv(int numberOfVertices, double *u, double *v);

    inline ColorRgb
    getRadianceAtPixel(
        Camera *camera,
        int x,
        int y,
        Patch *patch,
        RadianceMethod *context,
        RenderOptions *renderOptions);

  public:
    explicit RayCaster(ScreenBuffer *inScreen, Camera *defaultCamera);
    virtual ~RayCaster();
    void render(Scene *scene, RadianceMethod *context, RenderOptions *renderOptions);
    void display();
    void save(ImageOutputHandle *ip);
};

#ifdef RAYTRACING_ENABLED
    extern Raytracer GLOBAL_rayCasting_RayCasting;
#endif

extern void
rayCast(
    char *fileName,
    FILE *fp,
    int isPipe,
    Scene *scene,
    RadianceMethod *context,
    RenderOptions *renderOptions);

#endif
