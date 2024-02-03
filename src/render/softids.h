/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef _RPK_SOFTIDS_H_
#define _RPK_SOFTIDS_H_

#include "java/util/ArrayList.h"
#include "SGL/sgl.h"
#include "skin/Patch.h"

extern SGL_CONTEXT *setupSoftFrameBuffer();

extern void
softRenderPatches(
    SGL_PIXEL (*patch_pixel)(Patch *),
    java::ArrayList<Patch *> *scenePatches);

extern unsigned long *
softRenderIds(
    long *x,
    long *y,
    java::ArrayList<Patch *> *scenePatches);

extern unsigned long *
sglRenderIds(long *x, long *y, java::ArrayList<Patch *> *scenePatches);

#endif
