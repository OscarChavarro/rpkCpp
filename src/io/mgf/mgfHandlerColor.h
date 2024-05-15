#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/mgf/MgfContext.h"

extern void initColorContextTables(MgfContext *context);
extern int handleColorEntity(int ac, const char **av, MgfContext * /*context*/);

#endif
