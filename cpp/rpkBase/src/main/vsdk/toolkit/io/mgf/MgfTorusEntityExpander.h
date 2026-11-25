#ifndef __MGF_TORUS_ENTITY_EXPANDER__
#define __MGF_TORUS_ENTITY_EXPANDER__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfTorusEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
