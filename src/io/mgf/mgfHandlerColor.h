#ifndef __MGF_HANDLER_COLOR__
#define __MGF_HANDLER_COLOR__

#include "io/mgf/MgfContext.h"

extern void initColorContextTables();
extern int handleColorEntity(int ac, char **av, MgfContext * /*context*/);

#endif