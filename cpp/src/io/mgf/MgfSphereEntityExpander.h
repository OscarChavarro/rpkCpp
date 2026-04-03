#ifndef __MGF_SPHERE_GEOMETRY__
#define __MGF_SPHERE_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfSphereEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
