#ifndef __MGF_CONE_GEOMETRY__
#define __MGF_CONE_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfConeGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
