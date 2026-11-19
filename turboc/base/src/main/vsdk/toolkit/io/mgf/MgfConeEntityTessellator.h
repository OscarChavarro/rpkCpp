#ifndef __MGF_CONE_GEOMETRY__
#define __MGF_CONE_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfConeEntityTessellator{ public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
