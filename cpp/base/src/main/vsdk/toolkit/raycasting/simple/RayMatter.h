#ifndef RAY_MATTER__
#define RAY_MATTER__

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/raycasting/common/PixelFilter.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/common/RayTracer.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterFilterType.h"

class RayMatter final : public RayTracer {
  private:
    static RayMatter *rayMatter;
    static constexpr char NAME[] = "Ray Matting";
    ScreenBuffer *screenBuffer;
    PixelFilter *pixelFilter;
    bool doDeleteScreen;
    RayMatterState &rayMatterState;

  public:
    explicit RayMatter(
        ScreenBuffer *screen,
        const Camera *camera,
        RayMatterState &inRayMatterState,
        ToneMappingContext *toneMapOptions = nullptr);
    ~RayMatter() final;

    void createFilter();
    void doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid);
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
#endif
