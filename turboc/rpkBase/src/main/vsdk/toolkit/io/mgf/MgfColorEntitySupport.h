#ifndef __MGF_COLOR_ENTITY_SUPPORT__
#define __MGF_COLOR_ENTITY_SUPPORT__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfColorEntitySupport {
  public:
    static void initColorContextTables(ParseRuntimeContext *context);
    static int handleColorEntity(int ac, const char **av, ParseRuntimeContext *context);
};

#endif
