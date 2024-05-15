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

class RayMatter {
  private:
    ScreenBuffer *screenBuffer;
    PixelFilter *pixelFilter;
    bool doDeleteScreen;

  public:
    explicit RayMatter(ScreenBuffer *screen, Camera *camera);
    virtual ~RayMatter();

    void createFilter();
    void doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid);
    void display();
    void save(ImageOutputHandle *ip);
};

extern Raytracer GLOBAL_rayCasting_RayMatting;

#endif
