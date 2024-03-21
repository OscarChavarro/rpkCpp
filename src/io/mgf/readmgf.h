#ifndef __READ_MGF__
#define __READ_MGF__

#include "skin/RadianceMethod.h"

extern void readMgf(
    char *filename,
    RadianceMethod *context,
    bool singleSided);
extern void mgfFreeMemory();

#endif
