#ifndef __MGF_CYLINDER_GEOMETRY__
#define __MGF_CYLINDER_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfCylinderEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
