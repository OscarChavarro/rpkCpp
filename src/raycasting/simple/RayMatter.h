#ifndef __RAY_MATTER__
#define __RAY_MATTER__

#include "raycasting/common/PixelFilter.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"

enum RayMatterFilterType {
    BOX_FILTER,
    TENT_FILTER,
    GAUSS_FILTER,
    GAUSS2_FILTER
};

class RayMatterState {
public:
    int samplesPerPixel; // Pixel sampling
    RayMatterFilterType filter; // Pixel filter
};

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
};

extern Raytracer GLOBAL_rayCasting_RayMatting;

#endif
