#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/mgf/MgfContext.h"

extern void readMgf(const char *filename, MgfContext *context);
extern void mgfFreeMemory(MgfContext *context);

#endif
