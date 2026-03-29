#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/context/MgfParseSession.h"

extern void initColorContextTables(MgfParseSession *context);
extern int handleColorEntity(int ac, const char **av, MgfParseSession * /*context*/);

#endif
