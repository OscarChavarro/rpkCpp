
#include "common/VSDK.h"
#ifndef GLRKN_CLSTR_STRTG
#define GLRKN_CLSTR_STRTG

// Determines how source radiance of a source cluster is determined and
// how irradiance is distributed over the patches in a receiver cluster
enum GalerkinClusteringStrategy {
    ISOTROPIC,
    ORIENTED,
    Z_VISIBILITY
};

#endif
