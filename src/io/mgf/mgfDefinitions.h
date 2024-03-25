#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include <cstdio>

#include "io/mgf/MgfContext.h"

class MgfReaderFilePosition;

extern int mgfDefaultHandlerForUnknownEntities(int ac, char **av, MgfContext *context);
extern int mgfOpen(MgfReaderContext *readerContext, char *functionCallback, MgfContext *context);
extern void mgfClose(MgfContext *context);
extern void doError(const char *errmsg, MgfContext *context);
extern void doWarning(const char *errmsg, MgfContext *context);
extern void mgfGetFilePosition(MgfReaderFilePosition *pos, MgfContext *context);
extern int mgfGoToFilePosition(MgfReaderFilePosition *pos, MgfContext *context);
extern int mgfEntity(char *name, MgfContext *context);
extern int mgfHandle(int entityIndex, int argc, char **argv, MgfContext * /*context*/);

#include "io/mgf/MgfTransformContext.h"

#endif
