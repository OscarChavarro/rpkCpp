#ifndef __STOCHASTIC_RAYTRACING_CSCATTER_INFO__
#define __STOCHASTIC_RAYTRACING_CSCATTER_INFO__

#include "raycasting/common/pathnode.h"

/**
CScatterinfo includes information about different scattering properties for different bsdf components
This info is used during scattering, but also when weighting or reading storage decisions must be made
*/
class CScatterInfo {
  public:
    // The components under consideration
    char flags;
    // Spawning factor if no 'flags' bounce was made before
    int nrSamplesBefore;
    // Spawning factor after at least one 'flags' bounce
    int nrSamplesAfter;

    // Some utility functions

    // Were 'flags' last used in the bounce in 'node'
    bool
    DoneThisBounce(const SimpleRaytracingPathNode *node) const {
        return (node->m_usedComponents == flags);
    }

    // Were 'flags' used at some previous point in the path
    bool
    DoneSomePreviousBounce(const SimpleRaytracingPathNode *node) const {
        return ((node->m_accUsedComponents & flags) == flags);
    }
};

#endif
