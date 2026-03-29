#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/context/MgfParseSession.h"

extern int handleObjectEntity(int argc, const char **argv, MgfParseSession * /*context*/);
extern void mgfObjectNewSurface(MgfParseSession *context);
extern void mgfObjectSurfaceDone(MgfParseSession *context);
extern void mgfObjectFreeMemory();

#endif
