#ifndef __BI_DIRECTIONAL_PATH__
#define __BI_DIRECTIONAL_PATH__

#include "raycasting/common/Raytracer.h"

class BidirectionalPathRaytracer final : public RayTracer {
  private:
    static char name[];
  public:
    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;
};

extern Raytracer GLOBAL_raytracing_biDirectionalPathMethod;

#endif
