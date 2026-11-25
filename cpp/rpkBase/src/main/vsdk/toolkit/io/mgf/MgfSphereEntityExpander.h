#ifndef __MGF_SPHERE_ENTITY_EXPANDER__
#define __MGF_SPHERE_ENTITY_EXPANDER__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfSphereEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
