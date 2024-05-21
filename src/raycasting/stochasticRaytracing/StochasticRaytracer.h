#ifndef __STOCHASTIC_RAYTRACER__
#define __STOCHASTIC_RAYTRACER__

#include "raycasting/common/Raytracer.h"

class StochasticRaytracer final : public RayTracer {
  public:
    void defaults() final;
};

extern Raytracer GLOBAL_raytracing_stochasticMethod;

#endif
