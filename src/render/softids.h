/**
Software ID rendering: because hardware ID rendering is tricky 
due to frame buffer formats, etc.
*/

#ifndef __SOFT_IDS__
#define __SOFT_IDS__

#include "java/util/ArrayList.h"
#include "SGL/sgl.h"
#include "skin/Patch.h"

extern SGL_CONTEXT *setupSoftFrameBuffer();

extern void
softRenderPatches(
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry);

extern unsigned long *
softRenderIds(
    long *x,
    long *y,
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry);

extern unsigned long *
sglRenderIds(
    long *x,
    long *y,
    Camera *camera,
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry);

#endif
