#ifndef MGF_HANDLER_COLOR__
#define MGF_HANDLER_COLOR__

#include "io/context/ParseRuntimeContext.h"

class MgfColorEntitySupport {
  public:
    static void initColorContextTables(ParseRuntimeContext *context);
    static int handleColorEntity(int ac, const char **av, ParseRuntimeContext *context);
};

#endif
