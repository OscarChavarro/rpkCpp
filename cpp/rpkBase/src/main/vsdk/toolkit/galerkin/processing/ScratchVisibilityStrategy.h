/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#ifndef __SCRATCH_VISIBILITY_STRATEGY__
#define __SCRATCH_VISIBILITY_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/ScratchVisibilityWorkspace.h"

/**
Every operation works on the scratch workspace it is given, the solver's or a
parallel Jacobi worker's, and reads the solver state only
*/
class ScratchVisibilityStrategy {
  private:
    static void addSamples(
        ScratchVisibilityWorkspace *scratchWorkspace,
        const GalerkinElement *element,
        int samples,
        const GalerkinState *galerkinState);

  public:
    static void scratchInit(GalerkinState *galerkinState);
    static void scratchTerminate(GalerkinState *galerkinState);
    static RpkAxisAlignedBoundingBox scratchRenderElements(
        GalerkinElement *cluster, Vector3Dd eye, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static ColorRgb scratchRadiance(const ScratchVisibilityWorkspace *scratch, const GalerkinState *galerkinState);
    static int scratchNonBackgroundPixels(const ScratchVisibilityWorkspace *scratch);
    static void scratchPixelsPerElement(ScratchVisibilityWorkspace *scratch, const GalerkinState *galerkinState);
    static double scratchPixelArea(const RpkAxisAlignedBoundingBox &boundingBox, const ScratchVisibilityWorkspace *scratch);
};

#endif
