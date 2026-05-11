#ifndef MGF_RING_GEOMETRY__
#define MGF_RING_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfRingEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
