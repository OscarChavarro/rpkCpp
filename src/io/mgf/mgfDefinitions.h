#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include <cstdio>

class MgfContext;

class MgdReaderFilePosition {
  public:
    int fid; // File this position is for
    int lineno; // Line number in file
    long offset; // Offset from beginning
};

#define MGF_MAXIMUM_INPUT_LINE_LENGTH 4096
#define MGF_MAXIMUM_ARGUMENT_COUNT (MGF_MAXIMUM_INPUT_LINE_LENGTH / 4)

class MgfReaderContext {
  public:
    char fileName[96];
    FILE *fp; // stream pointer
    int fileContextId;
    char inputLine[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    int lineNumber;
    char isPipe; // Flag indicating whether input comes from a pipe or a real file
    MgfReaderContext *prev; // Previous context
};

extern int mgfOpen(MgfReaderContext *readerContext, char *functionCallback, MgfContext *context);
extern void mgfClose(MgfContext *context);

extern void doError(const char *errmsg, MgfContext *context);
extern void doWarning(const char *errmsg, MgfContext *context);

extern void mgfGetFilePosition(MgdReaderFilePosition *pos, MgfContext *context);
extern int mgfGoToFilePosition(MgdReaderFilePosition *pos, MgfContext *context);
extern int mgfEntity(char *name, MgfContext *context);
extern int mgfHandle(int en, int ac, char **av, MgfContext * /*context*/);

#include "io/mgf/MgfContext.h"

#endif
