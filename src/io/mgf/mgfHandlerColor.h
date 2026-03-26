#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/context/BaseContext.h"

extern void initColorContextTables(BaseContext *context);
extern int handleColorEntity(int ac, const char **av, BaseContext * /*context*/);

#endif
