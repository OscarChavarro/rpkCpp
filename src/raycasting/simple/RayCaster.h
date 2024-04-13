#ifndef __RAY_CASTER__
#define __RAY_CASTER__

#include "common/RenderOptions.h"
#include "java/util/ArrayList.h"
#include "skin/RadianceMethod.h"
#include "render/ScreenBuffer.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/common/Raytracer.h"
#endif

class RayCaster {
  private:
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;

    static void clipUv(int numberOfVertices, double *u, double *v);

    inline ColorRgb getRadianceAtPixel(int x, int y, Patch *patch, RadianceMethod *context);

  public:
    explicit RayCaster(ScreenBuffer *inScreen);
    virtual ~RayCaster();
    void render(java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry, RadianceMethod *context);
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
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context);

#endif
