#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/mgf/MgfContext.h"

extern int handleObjectEntity(int argc, char **argv, MgfContext * /*context*/);
extern void mgfObjectNewSurface(MgfContext *context);
extern void mgfObjectSurfaceDone(MgfContext *context);
extern void mgfObjectFreeMemory();
extern void mgfObjectFreeLists(MgfContext *context);

#endif
