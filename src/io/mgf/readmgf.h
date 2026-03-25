#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/mgf/MgfContext.h"
#include "io/mgf/MgfModel.h"

extern MgfModel *readMgf(const char *filename, MgfContext *context);
extern void mgfFreeMemory(MgfContext *context);

#endif
