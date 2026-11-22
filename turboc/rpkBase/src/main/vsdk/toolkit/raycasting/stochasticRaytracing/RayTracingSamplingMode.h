#ifndef __RAYTRACING_SAMPLING_MODE__
#define __RAYTRACING_SAMPLING_MODE__

#include "vsdk/toolkit/common/VSDK.h"

enum RayTracingSamplingMode {
    BRDF_SAMPLING,
    CLASSICAL_SAMPLING,
    PHOTON_MAP_SAMPLING
};

#endif
