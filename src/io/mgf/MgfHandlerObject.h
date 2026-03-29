#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/context/BaseContext.h"

extern int handleObjectEntity(int argc, const char **argv, BaseContext * /*context*/);
extern void mgfObjectNewSurface(BaseContext *context);
extern void mgfObjectSurfaceDone(BaseContext *context);
extern void mgfObjectFreeMemory();

#endif
