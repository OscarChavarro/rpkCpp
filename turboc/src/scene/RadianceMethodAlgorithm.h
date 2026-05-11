#ifndef __RADIANCE_METHOD_ALGORITHM__
#define __RADIANCE_METHOD_ALGORITHM__

#include "material/RendererConfiguration.h"

enum RadianceMethodAlgorithm {
    GALERKIN
#ifdef RAYTRACING_ENABLED
    ,
    STOCHASTIC_JACOBI,
    RANDOM_WALK,
    PHOTON_MAP
#endif
};

#endif
