#ifndef __MGF_RING_GEOMETRY__
#define __MGF_RING_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfRingEntityTessellator{ public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
