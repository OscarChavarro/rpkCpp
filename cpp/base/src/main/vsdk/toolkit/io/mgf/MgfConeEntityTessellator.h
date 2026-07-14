#ifndef MGF_CONE_GEOMETRY__
#define MGF_CONE_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfConeEntityTessellator final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
