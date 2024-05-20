/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#ifndef __SCRATCH__
#define __SCRATCH__

#include "GALERKIN/GalerkinState.h"

class ScratchVisibilityStrategy {
  public:
    static void scratchInit(GalerkinState *galerkinState);
    static void scratchTerminate(GalerkinState *galerkinState);
    static float *scratchRenderElements(GalerkinElement *cluster, Vector3D eye, GalerkinState *galerkinState);
    static ColorRgb scratchRadiance(const GalerkinState *galerkinState);
    static int scratchNonBackgroundPixels(const GalerkinState *galerkinState);
    static void scratchPixelsPerElement(const GalerkinState *galerkinState);
};

#endif
