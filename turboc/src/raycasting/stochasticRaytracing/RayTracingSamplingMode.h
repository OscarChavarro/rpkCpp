#ifndef __RAYTRACING_SAMPLING_MODE__
#define __RAYTRACING_SAMPLING_MODE__

#include "common/VSDK.h"

enum RayTracingSamplingMode {
    BRDF_SAMPLING,
    CLASSICAL_SAMPLING,
    PHOTON_MAP_SAMPLING
};

#endif
