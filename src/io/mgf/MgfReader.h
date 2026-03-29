#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/context/MgfParseSession.h"
#include "io/context/PersistedSceneModel.h"

extern PersistedSceneModel *readMgf(const char *filename, MgfParseSession *context);
extern void mgfFreeMemory(MgfParseSession *context);

#endif
