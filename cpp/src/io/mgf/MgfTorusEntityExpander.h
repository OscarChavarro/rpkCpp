#ifndef __MGF_TORUS_GEOMETRY__
#define __MGF_TORUS_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfTorusEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
