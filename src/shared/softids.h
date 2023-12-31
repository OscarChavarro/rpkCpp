/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef _RPK_SOFTIDS_H_
#define _RPK_SOFTIDS_H_

#include "SGL/sgl.h"
#include "skin/Patch.h"

/* sets up a software rendering context and initialises transforms and 
 * viewport for the current view. The new renderer is made current. */
extern SGL_CONTEXT *SetupSoftFrameBuffer();

/* renders all GLOBAL_scene_patches in the current sgl renderer. PatchPixel returns
 * and SGL_PIXEL value for a given Patch */
extern void SoftRenderPatches(SGL_PIXEL (*PatchPixel)(Patch *));

/* software ID rendering */
extern unsigned long *SoftRenderIds(long *x, long *y);

#endif
