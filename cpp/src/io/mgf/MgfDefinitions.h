#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/context/ParseSession.h"
#include "io/context/FilePositionContext.h"
#include "io/context/HandlerType.h"
#include "io/context/EntityHandler.h"

class MgfDefinitions {
  public:
    static int mgfOpen(ReaderContext *readerContext, const char *functionCallback, ParseSession *context);
    static void mgfClose(ParseSession *context);
    static void doError(const char *errmsg, ParseSession *context);
    static void doWarning(const char *errmsg, ParseSession *context);
    static void mgfGetFilePosition(FilePositionContext *pos, ParseSession *context);
    static int mgfGoToFilePosition(const FilePositionContext *pos, ParseSession *context);
    static int mgfEntity(const char *name, ParseSession *context);
    static int mgfHandle(int entityIndex, int argc, const char **argv, ParseSession *context);
    static void mgfLookUpFreeMemory(ParseSession *context);

  private:
    static const char *standardInputPath();
    static bool skipLines(java::InputStream *inputStream, int lineCount);
    static int mgfDefaultHandlerForUnknownEntities(int ac, const char **av, const ParseSession *context);
};

#include "io/context/TransformStackContext.h"

#endif
