#ifndef STOCHASTIC_RAYTRACING_LINK__
#define STOCHASTIC_RAYTRACING_LINK__

#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Link {
  public:
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
};

#endif
