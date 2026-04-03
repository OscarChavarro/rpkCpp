#ifndef __READ_MGF__
#define __READ_MGF__

#include "io/context/ParseRuntimeContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/EntityDispatchContext.h"
#include "io/context/HandlerRoleContext.h"

class MgfParserLoader {
  public:
    static ParseSnapshotContext *readMgf(const char *filename, ParseRuntimeContext *context);
    static void mgfFreeMemory(ParseRuntimeContext *context);

  private:
    static int readInputLine(java::InputStream *inputStream, char *readBuffer, int maxLength);
    static int mgfReadNextLine(const ParseRuntimeContext *context);
    static int mgfParseCurrentLine(ParseRuntimeContext *context);
    static void mgfClear(ParseRuntimeContext *context);
    static void mgfSetNrQuartCircDivs(int divs);
    static void mgfSetMonochrome(bool yesno, ParseRuntimeContext *context);
    static int mgfDiscardUnNeededEntity(int ac, const char **av, ParseRuntimeContext *context);
    static int mgfPutCSpec(ParseRuntimeContext *context);
    static int mgfPutCxy(ParseRuntimeContext *context);
    static int mgfECSpec(int ac, const char **av, ParseRuntimeContext *context);
    static int mgfECMix(int ac, const char **av, ParseRuntimeContext *context);
    static int mgfColorTemperature(int ac, const char **av, ParseRuntimeContext *context);
    static int handleIncludedFile(int ac, const char **av, ParseRuntimeContext *context);
    static void ensureSessionHandlerRegistry(ParseRuntimeContext *context);
    static EntityDispatchContext *mgfHandlerFromType(ParseRuntimeContext *context, HandlerRoleContext handlerType);
    static bool mgfHandlerMatches(const EntityDispatchContext *handler, HandlerRoleContext handlerType);
    static void mgfAlternativeInit(EntityDispatchContext *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES], ParseRuntimeContext *context);
    static void initMgf(ParseRuntimeContext *context);
    static ParseSnapshotContext *mgfBuildModel(ParseRuntimeContext *context);
};

#endif
