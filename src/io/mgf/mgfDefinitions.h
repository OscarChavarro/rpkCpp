#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include <cstdio>

#include "io/mgf/MgfContext.h"

class MgfReaderFilePosition;

typedef int (*HandleCallBack)(int, const char **, MgfContext *);

extern int mgfOpen(MgfReaderContext *readerContext, const char *functionCallback, MgfContext *context);
extern void mgfClose(MgfContext *context);
extern void doError(const char *errmsg, MgfContext *context);
extern void doWarning(const char *errmsg, MgfContext *context);
extern void mgfGetFilePosition(MgfReaderFilePosition *pos, MgfContext *context);
extern int mgfGoToFilePosition(const MgfReaderFilePosition *pos, MgfContext *context);
extern int mgfEntity(const char *name, MgfContext *context);
extern int mgfHandle(int entityIndex, int argc, const char **argv, MgfContext * /*context*/);
extern void mgfLookUpFreeMemory();

#include "io/mgf/MgfTransformContext.h"

#endif
