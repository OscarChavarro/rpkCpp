#ifndef __LINK__
#define __LINK__

#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class Link {
  public:
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
};

#endif
