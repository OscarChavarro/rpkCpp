#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/context/ParseSession.h"

class MgfHandlerColor {
  public:
    static void initColorContextTables(ParseSession *context);
    static int handleColorEntity(int ac, const char **av, ParseSession *context);
};

#endif
