#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/mgf/MgfContext.h"

extern int handleObjectEntity(int argc, char **argv, MgfContext * /*context*/);
extern void newSurface(MgfContext *context);
extern void surfaceDone(MgfContext *context);
extern void mgfFreeObjectMemory();

#endif
