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
    static AxisAlignedBoundingBox *scratchRenderElements(GalerkinElement *cluster, Vector3D eye, GalerkinState *galerkinState);
    static ColorRgbMutable scratchRadiance(const GalerkinState *galerkinState);
    static int scratchNonBackgroundPixels(const GalerkinState *galerkinState);
    static void scratchPixelsPerElement(const GalerkinState *galerkinState);
};

#endif
