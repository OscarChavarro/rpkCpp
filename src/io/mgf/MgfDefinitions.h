#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/context/MgfParseSession.h"
#include "io/mgf/MgfHandlerType.h"
#include "io/mgf/MgfEntityHandler.h"

class FilePositionContext;

extern MgfEntityHandler *mgfHandlerFromType(MgfHandlerType handlerType);
extern bool mgfHandlerMatches(const MgfEntityHandler *handler, MgfHandlerType handlerType);

extern int mgfOpen(ReaderContext *readerContext, const char *functionCallback, MgfParseSession *context);
extern void mgfClose(MgfParseSession *context);
extern void doError(const char *errmsg, MgfParseSession *context);
extern void doWarning(const char *errmsg, MgfParseSession *context);
extern void mgfGetFilePosition(FilePositionContext *pos, MgfParseSession *context);
extern int mgfGoToFilePosition(const FilePositionContext *pos, MgfParseSession *context);
extern int mgfEntity(const char *name, MgfParseSession *context);
extern int mgfHandle(int entityIndex, int argc, const char **argv, MgfParseSession * /*context*/);
extern void mgfLookUpFreeMemory();

#include "io/context/TransformStackContext.h"

#endif
