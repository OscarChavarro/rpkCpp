#ifndef MGF_TORUS_GEOMETRY__
#define MGF_TORUS_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfTorusEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
