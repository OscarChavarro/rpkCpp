#ifndef __RAY_CASTER__
#define __RAY_CASTER__

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "scene/RadianceMethod.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/RayTracer.h"

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
        const RenderOptions *renderOptions) const;

  public:
    explicit RayCaster(ScreenBuffer *inScreen, const Camera *defaultCamera, ToneMappingContext *toneMapOptions = nullptr);
    ~RayCaster() final;
    void render(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions);
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
        const RenderOptions *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const;

    static void
    rayCast(
        const char *fileName,
        java::OutputStream *stream,
        int isPipe,
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RenderOptions *renderOptions);
};

#endif
