/* scratch.h: scratch renderer routines. Used for handling intra-cluster visibility
 * with a Z-buffer visibility algorithm in software. */

#ifndef _SCRATCH_H_
#define _SCRATCH_H_

/* create a scratch software renderer for various operations on clusters. */
extern void ScratchInit();

/* terminates scratch rendering */
extern void ScratchTerminate();

/* Sets up an orthographic projection of the cluster as
 * seen from the eye. Renders pointers to the elements
 * in clus in the scratch frame buffer. Returns a boundingbox 
 * containing the size of the virtual screen, which nicely fits 
 * around clus. */
extern float *ScratchRenderElementPtrs(GalerkinElement *clus, Vector3D eye);

/* After rendering element pointers in the scratch frame buffer, this routine 
 * computes the average radiance or unshot radiance (depending on the
 * current ieration method) of the virtual screen. */
extern COLOR ScratchRadiance();

/* Computes the number of non background pixels. */
extern int ScratchNonBackgroundPixels();

/* Counts the number of pixels occupied by each element. The result is
 * accumulated in the tmp field of the elements. This field should be
 * initialized to zero before. */
extern void ScratchPixelsPerElement();

#endif
