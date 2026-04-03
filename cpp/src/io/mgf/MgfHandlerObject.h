#ifndef __MGF_HANDLER_OBJECT__
#define __MGF_HANDLER_OBJECT__

#include "io/context/ParseSession.h"

class MgfHandlerObject {
  public:
    static int handleObjectEntity(int argc, const char **argv, ParseSession *context);
    static void mgfObjectNewSurface(ParseSession *context);
    static void mgfObjectSurfaceDone(ParseSession *context);
    static void mgfObjectFreeMemory(ParseSession *context);

  private:
    static void disposeCurrentSurfaceLists(ParseSession *context);
    static void pushCurrentGeometryList(ParseSession *context);
    static void popCurrentGeometryList(ParseSession *context);
    static int handleObject2Entity(int ac, const char **av, ParseSession *context);
};

#endif
