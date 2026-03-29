#ifndef __MGF_DEFINITIONS__
#define __MGF_DEFINITIONS__

#include "io/context/MgfParseSession.h"
#include "io/mgf/MgfHandlerType.h"
#include "io/mgf/MgfEntityHandler.h"

namespace java {
namespace io {
class InputStream;
}
}

class FilePositionContext;

class MgfDefinitions {
  public:
    static int mgfOpen(ReaderContext *readerContext, const char *functionCallback, MgfParseSession *context);
    static void mgfClose(MgfParseSession *context);
    static void doError(const char *errmsg, MgfParseSession *context);
    static void doWarning(const char *errmsg, MgfParseSession *context);
    static void mgfGetFilePosition(FilePositionContext *pos, MgfParseSession *context);
    static int mgfGoToFilePosition(const FilePositionContext *pos, MgfParseSession *context);
    static int mgfEntity(const char *name, MgfParseSession *context);
    static int mgfHandle(int entityIndex, int argc, const char **argv, MgfParseSession *context);
    static void mgfLookUpFreeMemory(MgfParseSession *context);

  private:
    static const char *standardInputPath();
    static bool skipLines(java::io::InputStream *inputStream, int lineCount);
    static int mgfDefaultHandlerForUnknownEntities(int ac, const char **av, const MgfParseSession *context);
};

#include "io/context/TransformStackContext.h"

#endif
