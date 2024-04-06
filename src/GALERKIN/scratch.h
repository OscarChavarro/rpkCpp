/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#ifndef __SCRATCH__
#define __SCRATCH__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

extern void scratchInit(GalerkinState *galerkinState);
extern void scratchTerminate(GalerkinState *galerkinState);
extern float *scratchRenderElements(GalerkinElement *cluster, Vector3D eye, GalerkinState *galerkinState);
extern ColorRgb scratchRadiance(GalerkinState *galerkinState);
extern int scratchNonBackgroundPixels(GalerkinState *galerkinState);
extern void scratchPixelsPerElement();

#endif
