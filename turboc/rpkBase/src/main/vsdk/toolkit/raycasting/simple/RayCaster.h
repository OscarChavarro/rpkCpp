#ifndef __RAY_CASTER__
#define __RAY_CASTER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"

class RayCaster: public RayTracer{ private:
    static RayCaster *rayCaster;
    static const char *const NAME;
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;

    static void clipUv(int numberOfVertices, double *u, double *v);

    inline ColorRgb
    getRadianceAtPixel( Camera *camera, int x, int y, const Patch *patch, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) const;

  public:
    explicit RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera, ToneMappingContext *toneMapOptions = NULL);
    ~RayCaster();
    void
    render( const Scene *scene, const RadianceMethod *radianceMethod, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions);
    void display();
    void save(ImageOutputHandle *ip);

    void defaults();
    const char *getName() const;
    void initialize(const ArrayList<Patch *> *lightPatches) const;

    void
    execute( ImageOutputHandle *ip, Scene *scene, RadianceMethod *radianceMethod, ToneMappingContext *toneMapOptions, const RenderOptions *renderOptions) const;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const;
    void terminate() const;
};

#endif
