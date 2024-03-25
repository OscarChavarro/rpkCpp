#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include <cstdio>

#include "io/mgf/MgfContext.h"

class MgdReaderFilePosition {
  public:
    int fid; // File this position is for
    int lineno; // Line number in file
    long offset; // Offset from beginning
};

extern int mgfOpen(MgfReaderContext *readerContext, char *functionCallback, MgfContext *context);
extern void mgfClose(MgfContext *context);

extern void doError(const char *errmsg, MgfContext *context);
extern void doWarning(const char *errmsg, MgfContext *context);

extern void mgfGetFilePosition(MgdReaderFilePosition *pos, MgfContext *context);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos, MgfContext *context);
extern int mgfEntity(char *name, MgfContext *context);
extern int mgfHandle(int entityIndex, int argc, char **argv, MgfContext * /*context*/);

#endif
