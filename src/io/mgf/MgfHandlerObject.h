#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/context/MgfParseSession.h"

class MgfHandlerObject {
  public:
    static int handleObjectEntity(int argc, const char **argv, MgfParseSession *context);
    static void mgfObjectNewSurface(MgfParseSession *context);
    static void mgfObjectSurfaceDone(MgfParseSession *context);
    static void mgfObjectFreeMemory(MgfParseSession *context);

  private:
    static void disposeCurrentSurfaceLists(MgfParseSession *context);
    static void pushCurrentGeometryList(MgfParseSession *context);
    static void popCurrentGeometryList(MgfParseSession *context);
    static int handleObject2Entity(int ac, const char **av, MgfParseSession *context);
};

#endif
