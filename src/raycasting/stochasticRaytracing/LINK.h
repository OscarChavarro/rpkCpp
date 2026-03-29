#ifndef __STOCHASTIC_RAYTRACING_LINK__
#define __STOCHASTIC_RAYTRACING_LINK__

class StochasticRadiosityElement;

class LINK {
  public:
    StochasticRadiosityElement *rcv;
    StochasticRadiosityElement *src;
};

#endif
