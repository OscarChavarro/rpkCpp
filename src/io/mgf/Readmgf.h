#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/context/BaseContext.h"
#include "io/context/PersistedSceneModel.h"

extern PersistedSceneModel *readMgf(const char *filename, BaseContext *context);
extern void mgfFreeMemory(BaseContext *context);

#endif
