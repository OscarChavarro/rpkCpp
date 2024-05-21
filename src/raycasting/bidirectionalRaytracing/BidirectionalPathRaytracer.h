#ifndef __BI_DIRECTIONAL_PATH__
#define __BI_DIRECTIONAL_PATH__

#include "raycasting/common/Raytracer.h"

class BidirectionalPathRaytracer final : public RayTracer {
  public:
    void defaults() final;
};

extern Raytracer GLOBAL_raytracing_biDirectionalPathMethod;

#endif
