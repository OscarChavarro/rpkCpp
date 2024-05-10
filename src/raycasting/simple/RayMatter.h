#ifndef __RAY_MATTER__
#define __RAY_MATTER__

#include "raycasting/common/PixelFilter.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"

class RayMatter {
  private:
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;
    PixelFilter *pixelFilter;

  public:
    explicit RayMatter(ScreenBuffer *screen, Camera *camera);
    virtual ~RayMatter();

    void checkFilter();
    void doMatting(const Camera *camera, const VoxelGrid *sceneWorldVoxelGrid);
    void display();
    void save(ImageOutputHandle *ip);
};

extern Raytracer GLOBAL_rayCasting_RayMatting;

#endif
