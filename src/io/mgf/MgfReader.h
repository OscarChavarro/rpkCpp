#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/context/MgfParseSession.h"
#include "io/context/PersistedSceneModel.h"
#include "io/mgf/MgfEntityHandler.h"
#include "io/mgf/MgfHandlerType.h"

namespace java {
    namespace io {
        class InputStream;
    }
}

class MgfReader {
  public:
    static PersistedSceneModel *readMgf(const char *filename, MgfParseSession *context);
    static void mgfFreeMemory(MgfParseSession *context);

  private:
    static int readInputLine(java::io::InputStream *inputStream, char *readBuffer, int maxLength);
    static int mgfReadNextLine(const MgfParseSession *context);
    static int mgfParseCurrentLine(MgfParseSession *context);
    static void mgfClear(MgfParseSession *context);
    static void mgfSetNrQuartCircDivs(int divs);
    static void mgfSetMonochrome(bool yesno, MgfParseSession *context);
    static int mgfDiscardUnNeededEntity(int ac, const char **av, MgfParseSession *context);
    static int mgfPutCSpec(MgfParseSession *context);
    static int mgfPutCxy(MgfParseSession *context);
    static int mgfECSpec(int ac, const char **av, MgfParseSession *context);
    static int mgfECMix(int ac, const char **av, MgfParseSession *context);
    static int mgfColorTemperature(int ac, const char **av, MgfParseSession *context);
    static int handleIncludedFile(int ac, const char **av, MgfParseSession *context);
    static void ensureSessionHandlerRegistry(MgfParseSession *context);
    static MgfEntityHandler *mgfHandlerFromType(MgfParseSession *context, MgfHandlerType handlerType);
    static bool mgfHandlerMatches(const MgfEntityHandler *handler, MgfHandlerType handlerType);
    static void mgfAlternativeInit(MgfEntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES], MgfParseSession *context);
    static void initMgf(MgfParseSession *context);
    static PersistedSceneModel *mgfBuildModel(MgfParseSession *context);
};

#endif
