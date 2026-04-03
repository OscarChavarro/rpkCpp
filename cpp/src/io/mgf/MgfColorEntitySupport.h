#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/context/ParseRuntimeContext.h"

class MgfHandlerColor {
  public:
    static void initColorContextTables(ParseRuntimeContext *context);
    static int handleColorEntity(int ac, const char **av, ParseRuntimeContext *context);
};

#endif
