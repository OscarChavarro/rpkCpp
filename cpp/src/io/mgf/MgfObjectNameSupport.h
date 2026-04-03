#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/context/ParseRuntimeContext.h"

class MgfHandlerObject {
  public:
    static int handleObjectEntity(int argc, const char **argv, ParseRuntimeContext *context);
    static void mgfObjectNewSurface(ParseRuntimeContext *context);
    static void mgfObjectSurfaceDone(ParseRuntimeContext *context);
    static void mgfObjectFreeMemory(ParseRuntimeContext *context);

  private:
    static void disposeCurrentSurfaceLists(ParseRuntimeContext *context);
    static void pushCurrentGeometryList(ParseRuntimeContext *context);
    static void popCurrentGeometryList(ParseRuntimeContext *context);
    static int handleObject2Entity(int ac, const char **av, ParseRuntimeContext *context);
};

#endif
