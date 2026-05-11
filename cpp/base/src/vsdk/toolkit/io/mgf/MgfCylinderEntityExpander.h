#ifndef MGF_CYLINDER_GEOMETRY__
#define MGF_CYLINDER_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfCylinderEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
