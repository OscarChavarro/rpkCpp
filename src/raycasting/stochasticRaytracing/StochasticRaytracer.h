#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "raycasting/common/Raytracer.h"

class StochasticRaytracer final : public RayTracer {
  private:
    static char name[];
  public:
    void defaults() final;
    const char *getName() const final;
    void initialize(const java::ArrayList<Patch *> *lightPatches) const final;
};

extern Raytracer GLOBAL_raytracing_stochasticMethod;

#endif
