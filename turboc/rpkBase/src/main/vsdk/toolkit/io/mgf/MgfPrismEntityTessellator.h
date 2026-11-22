#ifndef __MGF_PRISM_GEOMETRY__
#define __MGF_PRISM_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfPrismEntityTessellator{ public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
