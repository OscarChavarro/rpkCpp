#ifndef __GALERKIN_SHAFT_CULL_MODE__
#define __GALERKIN_SHAFT_CULL_MODE__

#include "vsdk/toolkit/common/VSDK.h"

/**
When the Galerkin solver culls the candidate occluders of an interaction with
a shaft (RenderPark's SHAFTCULLMODE). With NEVER_DO_SHAFT_CULLING every shadow
ray that the shadow cache does not answer goes to the full-scene index.
*/
enum GalerkinShaftCullMode {
    ALWAYS_DO_SHAFT_CULLING,
    DO_SHAFT_CULLING_FOR_REFINEMENT,
    NEVER_DO_SHAFT_CULLING
};

#endif
