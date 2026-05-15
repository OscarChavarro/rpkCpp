#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/context/ParseRuntimeContext.h"
#include "io/context/FilePositionContext.h"
#include "io/context/HandlerRoleContext.h"
#include "io/context/EntityDispatchContext.h"

class MgfEntityControl {
  public:
    static int mgfOpen(ReaderContext *readerContext, const char *functionCallback, ParseRuntimeContext *context);
    static void mgfClose(ParseRuntimeContext *context);
    static void doError(const char *errmsg, ParseRuntimeContext *context);
    static void doWarning(const char *errmsg, ParseRuntimeContext *context);
    static void mgfGetFilePosition(FilePositionContext *pos, ParseRuntimeContext *context);
    static int mgfGoToFilePosition(const FilePositionContext *pos, ParseRuntimeContext *context);
    static int mgfEntity(const char *name, ParseRuntimeContext *context);
    static int mgfHandle(int entityIndex, int argc, const char **argv, ParseRuntimeContext *context);
    static void mgfLookUpFreeMemory(ParseRuntimeContext *context);

  private:
    static const char *standardInputPath();
    static bool skipLines(InputStream *inputStream, int lineCount);
    static int mgfDfltHndlrFUnknwEntts(int ac, const char **av, const ParseRuntimeContext *context);
};

#include "io/context/TransformStackContext.h"

#endif
