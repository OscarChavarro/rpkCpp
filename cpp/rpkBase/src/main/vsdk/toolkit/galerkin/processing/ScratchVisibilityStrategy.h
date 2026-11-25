/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#ifndef SCRATCH_VISIBILITY_STRATEGY__
#define SCRATCH_VISIBILITY_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinState.h"

class ScratchVisibilityStrategy {
  public:
    static void scratchInit(GalerkinState *galerkinState);
    static void scratchTerminate(GalerkinState *galerkinState);
    static const RpkAxisAlignedBoundingBox *scratchRenderElements(GalerkinElement *cluster, Vector3Df eye, GalerkinState *galerkinState);
    static ColorRgb scratchRadiance(const GalerkinState *galerkinState);
    static int scratchNonBackgroundPixels(const GalerkinState *galerkinState);
    static void scratchPixelsPerElement(GalerkinState *galerkinState);
};

#endif
