#ifndef __RAY_MATTER__
#define __RAY_MATTER__

#include "raycasting/common/pixelfilter.h"
#include "render/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"

class RayMatter {
  private:
    ScreenBuffer *screenBuffer;
    bool doDeleteScreen;
    pixelFilter *Filter;

  public:
    explicit RayMatter(ScreenBuffer *screen);
    virtual ~RayMatter();

    void CheckFilter();
    void Matting();
    void display();
    void save(ImageOutputHandle *ip);
};

extern Raytracer GLOBAL_rayCasting_RayMatting;

#endif
