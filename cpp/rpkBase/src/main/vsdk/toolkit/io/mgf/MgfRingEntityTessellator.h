#ifndef __MGF_RING_ENTITY_TESSELLATOR__
#define __MGF_RING_ENTITY_TESSELLATOR__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfRingEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
