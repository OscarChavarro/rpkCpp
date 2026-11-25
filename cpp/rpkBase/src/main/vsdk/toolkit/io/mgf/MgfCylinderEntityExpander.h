#ifndef __MGF_CYLINDER_ENTITY_EXPANDER__
#define __MGF_CYLINDER_ENTITY_EXPANDER__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfCylinderEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
