#ifndef RAY_CASTER__
#define RAY_CASTER__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"

class RayCaster final : public RayTracer {
  private:
    static RayCaster *rayCaster;
    static const char *const NAME;
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;

    static void clipUv(int numberOfVertices, double *u, double *v);

    inline ColorRgb
    getRadianceAtPixel(
        Camera *camera,
        int x,
        int y,
        Patch *patch,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions) const;

  public:
    explicit RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera, ToneMappingContext *toneMapOptions = nullptr);
    ~RayCaster() final;
    void
    render(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions);
    void display();
    void save(ImageOutputHandle *ip);

    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;

    void
    execute(
        ImageOutputHandle *ip,
        Scene *scene,
        RadianceMethod *radianceMethod,
        ToneMappingContext *toneMapOptions,
        const RendererConfiguration *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const;
};

#endif
