#ifndef __LD_SPAR__
#define __LD_SPAR__

#include "raycasting/bidirectionalRaytracing/Spar.h"

/**
LD Spar: Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class LDSpar: public Spar{ public:
    void init(SparConfig *sparConfig, RadianceMethod *radianceMethod);
};

#endif
