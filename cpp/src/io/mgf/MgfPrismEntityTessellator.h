#ifndef MGF_PRISM_GEOMETRY__
#define MGF_PRISM_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfPrismEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
