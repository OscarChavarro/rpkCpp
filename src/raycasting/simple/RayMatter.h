#ifndef _RAYMATTE_H_
#define _RAYMATTE_H_

#include "raycasting/common/pixelfilter.h"
#include "raycasting/common/ScreenBuffer.h"
#include "raycasting/common/Raytracer.h"

class RayMatter {
  private:
    ScreenBuffer *scrn;
    bool interrupt_requested;
    bool doDeleteScreen;
    pixelFilter *Filter;

  public:
    RayMatter(ScreenBuffer *screen);
    virtual ~RayMatter();

    void CheckFilter();
    void Matting();
    void display();
    void save(ImageOutputHandle *ip);
    void interrupt();
};

extern Raytracer GLOBAL_rayCasting_RayMatting;

#endif
