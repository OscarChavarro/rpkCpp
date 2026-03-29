#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/context/MgfParseSession.h"

class MgfHandlerColor {
  public:
    static void initColorContextTables(MgfParseSession *context);
    static int handleColorEntity(int ac, const char **av, MgfParseSession *context);
};

#endif
