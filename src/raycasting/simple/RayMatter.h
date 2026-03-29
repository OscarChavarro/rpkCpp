#ifndef __RAY_MATTER__
#define __RAY_MATTER__

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/common/PixelFilter.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/simple/RayMatterFilterType.h"

extern RayMatterState GLOBAL_rayCasting_rayMatterState;

class RayMatter final : public RayTracer {
  private:
    static char name[];
    ScreenBuffer *screenBuffer;
    PixelFilter *pixelFilter;
    bool doDeleteScreen;

  public:
    explicit RayMatter(ScreenBuffer *screen, const Camera *camera);
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
        const RenderOptions *renderOptions) const final;

    bool saveImage(ImageOutputHandle *imageOutputHandle) const final;
    void terminate() const;
};

#endif
#endif
