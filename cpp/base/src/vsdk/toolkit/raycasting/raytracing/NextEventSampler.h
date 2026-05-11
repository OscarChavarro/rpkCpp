#ifndef C_NEXT_EVENT_SAMPLER__
#define C_NEXT_EVENT_SAMPLER__

#include "vsdk/toolkit/raycasting/raytracing/Sampler.h"

/**
Next event samplers provide a few functions to
enumerate different 'next event units' (e.g. light sources
or cameras). This allows to sample all units separately,
f.i. if you want to sample all light sources.

The interface is very simple. I just wanted to be able
to sample all light sources.
*/
class NextEventSampler : public Sampler {
  public:
    // Setting units causes sampling of the activated unit
    // instead of over all units.

    virtual bool ActivateFirstUnit() { return false; }

    // Activate next unit.
    // If no next unit is available:
    //   Returns false and unsets units
    virtual bool ActivateNextUnit() { return false; }
};

#endif
