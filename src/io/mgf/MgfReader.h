#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/context/ParseSession.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/EntityHandler.h"
#include "io/context/HandlerType.h"

namespace java {
    namespace io {
        class InputStream;
    }
}

class MgfReader {
  public:
    static PersistedSceneModel *readMgf(const char *filename, ParseSession *context);
    static void mgfFreeMemory(ParseSession *context);

  private:
    static int readInputLine(java::io::InputStream *inputStream, char *readBuffer, int maxLength);
    static int mgfReadNextLine(const ParseSession *context);
    static int mgfParseCurrentLine(ParseSession *context);
    static void mgfClear(ParseSession *context);
    static void mgfSetNrQuartCircDivs(int divs);
    static void mgfSetMonochrome(bool yesno, ParseSession *context);
    static int mgfDiscardUnNeededEntity(int ac, const char **av, ParseSession *context);
    static int mgfPutCSpec(ParseSession *context);
    static int mgfPutCxy(ParseSession *context);
    static int mgfECSpec(int ac, const char **av, ParseSession *context);
    static int mgfECMix(int ac, const char **av, ParseSession *context);
    static int mgfColorTemperature(int ac, const char **av, ParseSession *context);
    static int handleIncludedFile(int ac, const char **av, ParseSession *context);
    static void ensureSessionHandlerRegistry(ParseSession *context);
    static EntityHandler *mgfHandlerFromType(ParseSession *context, HandlerType handlerType);
    static bool mgfHandlerMatches(const EntityHandler *handler, HandlerType handlerType);
    static void mgfAlternativeInit(EntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES], ParseSession *context);
    static void initMgf(ParseSession *context);
    static PersistedSceneModel *mgfBuildModel(ParseSession *context);
};

#endif
