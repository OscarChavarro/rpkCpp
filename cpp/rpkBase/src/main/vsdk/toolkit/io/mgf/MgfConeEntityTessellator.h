#ifndef __MGF_CONE_ENTITY_TESSELLATOR__
#define __MGF_CONE_ENTITY_TESSELLATOR__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfConeEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
