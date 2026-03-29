#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/context/BaseContext.h"
#include "io/mgf/MgfHandlerType.h"
#include "io/mgf/MgfEntityHandler.h"

class FilePositionContext;

extern MgfEntityHandler *mgfHandlerFromType(MgfHandlerType handlerType);
extern bool mgfHandlerMatches(const MgfEntityHandler *handler, MgfHandlerType handlerType);

extern int mgfOpen(ReaderContext *readerContext, const char *functionCallback, BaseContext *context);
extern void mgfClose(BaseContext *context);
extern void doError(const char *errmsg, BaseContext *context);
extern void doWarning(const char *errmsg, BaseContext *context);
extern void mgfGetFilePosition(FilePositionContext *pos, BaseContext *context);
extern int mgfGoToFilePosition(const FilePositionContext *pos, BaseContext *context);
extern int mgfEntity(const char *name, BaseContext *context);
extern int mgfHandle(int entityIndex, int argc, const char **argv, BaseContext * /*context*/);
extern void mgfLookUpFreeMemory();

#include "io/context/TransformStackContext.h"

#endif
