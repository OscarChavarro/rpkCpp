#ifndef __MGF_PRISM_ENTITY_TESSELLATOR__
#define __MGF_PRISM_ENTITY_TESSELLATOR__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfPrismEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
