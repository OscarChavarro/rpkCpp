/**
Scratch renderer routines. Used for handling intra-cluster visibility
with a Z-buffer visibility algorithm in software
*/

#ifndef __SCRATCH__
#define __SCRATCH__

extern void scratchInit();
extern void scratchTerminate();
extern float *scratchRenderElements(GalerkinElement *cluster, Vector3D eye);
extern COLOR scratchRadiance();
extern int scratchNonBackgroundPixels();
extern void scratchPixelsPerElement();

#endif
