#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/mgf/MgfContext.h"

extern void
readMgf(
    char *filename,
    MgfContext *context,
    bool singleSided);

extern void mgfFreeMemory();

#endif
