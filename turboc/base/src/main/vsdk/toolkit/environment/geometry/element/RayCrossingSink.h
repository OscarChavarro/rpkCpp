#ifndef __RAY_CROSSING_SINK__
#define __RAY_CROSSING_SINK__

#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"

class RayCrossingSink {
public:
    virtual ~RayCrossingSink();
    virtual void crossing(const RayCrossing& event) = 0;
};

#endif
